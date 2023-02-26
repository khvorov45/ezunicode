#include "cbuild.h"

#define STR(x) prb_STR(x)
#define LIT(x) prb_LIT(x)
#define assert(x) prb_assert(x)
#define function static

typedef prb_Str   Str;
typedef prb_Arena Arena;
typedef int32_t   i32;
typedef uint8_t   u8;
typedef intptr_t  isize;

function Str
readFile(Arena* arena, Str path) {
    prb_ReadEntireFileResult readres = prb_readEntireFile(arena, path);
    assert(readres.success);
    Str result = prb_strFromBytes(readres.content);
    return result;
}

int
main() {
    Arena  arena_ = prb_createArenaFromVmem(1 * prb_GIGABYTE);
    Arena* arena = &arena_;

    Str srcDir = prb_getParentDir(arena, STR(__FILE__));

    // NOTE(khvorov) Stb truetype
    Str stbttheader = {};
    Str stbttbody = {};    
    {
        Str            stbttFileContent = readFile(arena, prb_pathJoin(arena, srcDir, STR("stb_truetype.h")));
        prb_StrScanner stbttScanner = prb_createStrScanner(stbttFileContent);

        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#define __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#endif // __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
        stbttheader = prb_strTrim(stbttScanner.betweenLastMatches);

        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#ifdef STB_TRUETYPE_IMPLEMENTATION")}, prb_StrScannerSide_AfterMatch));
        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#endif // STB_TRUETYPE_IMPLEMENTATION")}, prb_StrScannerSide_AfterMatch));
        stbttbody = prb_strTrim(stbttScanner.betweenLastMatches);
    }

    // NOTE(khvorov) Font data
    Str fontData = {};
    {
        Str* allTTFFileContents = 0;
        Str* allFontIds = 0;
        {
            Str* entries = prb_getAllDirEntries(arena, srcDir, prb_Recursive_No);
            for (i32 entryIndex = 0; entryIndex < arrlen(entries); entryIndex++) {
                Str entry = entries[entryIndex];
                if (prb_strEndsWith(entry, STR(".ttf"))) {
                    Str name = {};
                    {
                        Str            filename = prb_getLastEntryInPath(entry);
                        prb_GrowingStr gstr = prb_beginStr(arena);
                        for (i32 byteIndex = 0; byteIndex < filename.len - 4; byteIndex++) {
                            char ch = filename.ptr[byteIndex];
                            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                                prb_addStrSegment(&gstr, "%c", ch);
                            }
                        }
                        name = prb_endStr(&gstr);
                    }
                    arrput(allFontIds, name);
                    Str content = readFile(arena, entry);
                    arrput(allTTFFileContents, content);
                }
            }
        }

        assert(arrlen(allTTFFileContents) == arrlen(allFontIds));

        {
            prb_GrowingStr gstr = prb_beginStr(arena);
            for (i32 ind = 0; ind < arrlen(allFontIds); ind++) {
                Str fontid = allFontIds[ind];
                Str data = allTTFFileContents[ind];
                prb_addStrSegment(&gstr, "uint8_t ezu_FontData_%.*s[] = {\n    ", LIT(fontid));
                for (i32 byteIndex = 0; byteIndex < data.len; byteIndex++) {
                    u8 byte = (u8)data.ptr[byteIndex];
                    prb_addStrSegment(&gstr, "0x%x,", byte);
                    if ((byteIndex == data.len - 1) || (byteIndex > 0 && (byteIndex % 30 == 0))) {
                        prb_addStrSegment(&gstr, "\n");
                        if (byteIndex != data.len - 1) {
                            prb_addStrSegment(&gstr, "    ");
                        }
                    } else {
                        prb_addStrSegment(&gstr, " ");
                    }
                }
                prb_addStrSegment(&gstr, "};");
            }
            fontData = prb_endStr(&gstr);
        }
    }

    Str ezunicodeMainFilePath = prb_pathJoin(arena, srcDir, STR("ezunicode.h"));
    Str ezunicodeMainFileContent = readFile(arena, ezunicodeMainFilePath);

    // NOTE(khvorov) Public functions
    Str coreheader = {};
    {
        prb_GrowingStr gstr = prb_beginStr(arena);
        prb_StrScanner scanner = prb_createStrScanner(ezunicodeMainFileContent);
        assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("ezu_PUBLICAPI")}, prb_StrScannerSide_AfterMatch));
        assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("ezu_PUBLICAPI")}, prb_StrScannerSide_AfterMatch));
        while (prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("ezu_PUBLICAPI")}, prb_StrScannerSide_AfterMatch)) {
            prb_addStrSegment(&gstr, "%.*s", LIT(scanner.match));
            assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.mode = prb_StrFindMode_LineBreak}, prb_StrScannerSide_AfterMatch));
            prb_addStrSegment(&gstr, "%.*s", LIT(scanner.betweenLastMatches));
            assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR(" {")}, prb_StrScannerSide_AfterMatch));
            prb_addStrSegment(&gstr, " %.*s;\n", LIT(scanner.betweenLastMatches));
        }
        coreheader = prb_endStr(&gstr);
    }

    // TODO(khvorov) Align header decls?

    // NOTE(khvorov) New content
    Str outContent = {};
    {
        prb_GrowingStr gstr = prb_beginStr(arena);
        prb_StrScanner scanner = prb_createStrScanner(ezunicodeMainFileContent);
        while (prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("// @")}, prb_StrScannerSide_AfterMatch)) {
            prb_addStrSegment(&gstr, "%.*s", LIT(scanner.betweenLastMatches));
            assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.mode = prb_StrFindMode_LineBreak, .alwaysMatchEnd = true}, prb_StrScannerSide_AfterMatch));
            Str line = scanner.betweenLastMatches;

            if (prb_streq(line, STR("stbttheader"))) {
                prb_addStrSegment(&gstr, "%.*s\n", LIT(stbttheader));
            } else if (prb_streq(line, STR("stbttbody"))) {
                prb_addStrSegment(&gstr, "%.*s\n", LIT(stbttbody));
            } else if (prb_streq(line, STR("fontdata"))) {
                prb_addStrSegment(&gstr, "%.*s\n", LIT(fontData));
            } else if (prb_streq(line, STR("coreheader"))) {
                prb_addStrSegment(&gstr, "%.*s", LIT(coreheader));
            } else {
                assert(!"unrecognized");
            }
        }
        prb_addStrSegment(&gstr, "%.*s", LIT(scanner.afterMatch));
        outContent = prb_endStr(&gstr);
    }

    Str rootDir = prb_getParentDir(arena, srcDir);
    Str ezunicodeOutPath = prb_pathJoin(arena, rootDir, STR("ezunicode.h"));
    assert(prb_writeEntireFile(arena, ezunicodeOutPath, outContent.ptr, outContent.len));

    return 0;
}
