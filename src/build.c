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

function void
insertBetween(prb_GrowingStr* dest, prb_StrScanner* scanner, Str insertion, Str from, Str to) {
    assert(prb_strScannerMove(scanner, (prb_StrFindSpec) {.pattern = from}, prb_StrScannerSide_AfterMatch));
    prb_addStrSegment(dest, "%.*s%.*s", LIT(scanner->betweenLastMatches), LIT(scanner->match));
    prb_addStrSegment(dest, "\n%.*s\n", LIT(insertion));

    assert(prb_strScannerMove(scanner, (prb_StrFindSpec) {.pattern = to}, prb_StrScannerSide_AfterMatch));
    prb_addStrSegment(dest, "%.*s", LIT(scanner->match));
}

function void
skipTo(prb_GrowingStr* dest, prb_StrScanner* scanner, Str pattern) {
    assert(prb_strScannerMove(scanner, (prb_StrFindSpec) {.pattern = pattern}, prb_StrScannerSide_AfterMatch));
    prb_addStrSegment(dest, "%.*s%.*s", LIT(scanner->beforeMatch), LIT(scanner->match));
}

int
main() {
    Arena  arena_ = prb_createArenaFromVmem(1 * prb_GIGABYTE);
    Arena* arena = &arena_;

    Str srcDir = prb_getParentDir(arena, STR(__FILE__));
    Str rootDir = prb_getParentDir(arena, srcDir);
    Str ezunicodeMainFilePath = prb_pathJoin(arena, rootDir, STR("ezunicode.h"));
    Str ezunicodeMainFileContent = readFile(arena, ezunicodeMainFilePath);

    // NOTE(khvorov) Stb truetype
    {
        Str            stbttFileContent = readFile(arena, prb_pathJoin(arena, srcDir, STR("stb_truetype.h")));
        prb_StrScanner stbttScanner = prb_createStrScanner(stbttFileContent);

        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#define __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#endif // __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
        Str stbttheader = prb_strTrim(stbttScanner.betweenLastMatches);

        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#ifdef STB_TRUETYPE_IMPLEMENTATION")}, prb_StrScannerSide_AfterMatch));
        assert(prb_strScannerMove(&stbttScanner, (prb_StrFindSpec) {.pattern = STR("#endif // STB_TRUETYPE_IMPLEMENTATION")}, prb_StrScannerSide_AfterMatch));
        Str stbttbody = prb_strTrim(stbttScanner.betweenLastMatches);

        prb_GrowingStr gstr = prb_beginStr(arena);
        prb_StrScanner mainScanner = prb_createStrScanner(ezunicodeMainFileContent);
        // NOTE(khvorov) Skip the integration section
        skipTo(&gstr, &mainScanner, STR("#endif  // ezu_USE_STB_TRUETYPE"));
        insertBetween(&gstr, &mainScanner, stbttheader, STR("#ifdef ezu_USE_STB_TRUETYPE"), STR("#endif  // ezu_USE_STB_TRUETYPE"));
        insertBetween(&gstr, &mainScanner, stbttbody, STR("#ifdef ezu_USE_STB_TRUETYPE"), STR("#endif  // ezu_USE_STB_TRUETYPE"));

        prb_addStrSegment(&gstr, "%.*s", LIT(mainScanner.afterMatch));
        ezunicodeMainFileContent = prb_endStr(&gstr);
    }

    // NOTE(khvorov) Font data
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

        // Str enumContent = {};
        // {
        //     prb_GrowingStr gstr = prb_beginStr(arena);
        //     for (i32 ind = 0; ind < arrlen(allFontIds); ind++) {
        //         Str fontid = allFontIds[ind];
        //         prb_addStrSegment(&gstr, "    ezu_FontID_%.*s,", LIT(fontid));
        //         if (ind < arrlen(allFontIds) - 1) {
        //             prb_addStrSegment(&gstr, "\n");
        //         }
        //     }
        //     enumContent = prb_endStr(&gstr);
        // }

        Str fontData = {};
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

        prb_GrowingStr gstr = prb_beginStr(arena);
        prb_StrScanner  mainScanner = prb_createStrScanner(ezunicodeMainFileContent);
        insertBetween(&gstr, &mainScanner, fontData, STR("#ifdef ezu_INCLUDE_FONT_DATA"), STR("#endif  // ezu_INCLUDE_FONT_DATA"));

        prb_addStrSegment(&gstr, "%.*s", LIT(mainScanner.afterMatch));
        ezunicodeMainFileContent = prb_endStr(&gstr);
    }

    assert(prb_writeEntireFile(arena, ezunicodeMainFilePath, ezunicodeMainFileContent.ptr, ezunicodeMainFileContent.len));

    return 0;
}
