#include "cbuild.h"

#define STR(x) prb_STR(x)
#define LIT(x) prb_LIT(x)
#define assert(x) prb_assert(x)
#define function static

typedef prb_Str   Str;
typedef prb_Arena Arena;
typedef int32_t   i32;

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
    Str rootDir = prb_getParentDir(arena, srcDir);
    Str ezunicodeMainFilePath = prb_pathJoin(arena, rootDir, STR("ezunicode.h"));
    Str ezunicodeMainFileContent = readFile(arena, ezunicodeMainFilePath);

    Str* ezunicodeNewFileContent = 0;

    // NOTE(khvorov) Stb truetype
    {
        Str stbttFileContent = readFile(arena, prb_pathJoin(arena, srcDir, STR("stb_truetype.h")));
        Str stbttheader = {};
        {
            prb_StrScanner scanner = prb_createStrScanner(stbttFileContent);
            assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("#define __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
            assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("#endif // __STB_INCLUDE_STB_TRUETYPE_H__")}, prb_StrScannerSide_AfterMatch));
            stbttheader = prb_strTrim(scanner.betweenLastMatches);
        }

        prb_StrScanner scanner = prb_createStrScanner(ezunicodeMainFileContent);
        assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("#ifdef ezu_USE_STB_TRUETYPE")}, prb_StrScannerSide_AfterMatch));
        arrput(ezunicodeNewFileContent, scanner.betweenLastMatches);
        arrput(ezunicodeNewFileContent, scanner.match);

        arrput(ezunicodeNewFileContent, STR("\n"));
        arrput(ezunicodeNewFileContent, stbttheader);
        arrput(ezunicodeNewFileContent, STR("\n"));

        assert(prb_strScannerMove(&scanner, (prb_StrFindSpec) {.pattern = STR("#endif  // ezu_USE_STB_TRUETYPE")}, prb_StrScannerSide_AfterMatch));
        arrput(ezunicodeNewFileContent, scanner.match);
        arrput(ezunicodeNewFileContent, scanner.afterMatch);
    }

    {
        prb_GrowingStr gstr = prb_beginStr(arena);
        for (i32 ind = 0; ind < arrlen(ezunicodeNewFileContent); ind++) {
            Str piece = ezunicodeNewFileContent[ind];
            prb_addStrSegment(&gstr, "%.*s", LIT(piece));
        }
        Str str = prb_endStr(&gstr);
        assert(prb_writeEntireFile(arena, ezunicodeMainFilePath, str.ptr, str.len));
    }

    return 0;
}
