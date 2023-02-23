#include "cbuild.h"

#define STR(x) prb_STR(x)
#define LIT(x) prb_LIT(x)
#define assert(x) prb_assert(x)
#define function static

typedef prb_Str   Str;
typedef prb_Arena Arena;
typedef int32_t   i32;
typedef intptr_t  isize;

function Str
readFile(Arena* arena, Str path) {
    prb_ReadEntireFileResult readres = prb_readEntireFile(arena, path);
    assert(readres.success);
    Str result = prb_strFromBytes(readres.content);
    return result;
}

typedef struct StrBuilder {
    Arena          arena;
    prb_GrowingStr gstr;
} StrBuilder;

function StrBuilder*
beginStr(Arena* arena, isize bytes) {
    StrBuilder* builder = prb_arenaAllocStruct(arena, StrBuilder);
    builder->arena = prb_createArenaFromArena(arena, bytes);
    builder->gstr = prb_beginStr(&builder->arena);
    return builder;
}

function void
insertBetween(prb_GrowingStr* dest, prb_StrScanner* scanner, Str insertion, Str from, Str to) {
    assert(prb_strScannerMove(scanner, (prb_StrFindSpec) {.pattern = from}, prb_StrScannerSide_AfterMatch));
    prb_addStrSegment(dest, "%.*s%.*s", LIT(scanner->betweenLastMatches), LIT(scanner->match));
    prb_addStrSegment(dest, "\n%.*s\n", LIT(insertion));

    assert(prb_strScannerMove(scanner, (prb_StrFindSpec) {.pattern = to}, prb_StrScannerSide_AfterMatch));
    prb_addStrSegment(dest, "%.*s", LIT(scanner->match));
}

int
main() {
    Arena  arena_ = prb_createArenaFromVmem(1 * prb_GIGABYTE);
    Arena* arena = &arena_;

    Str srcDir = prb_getParentDir(arena, STR(__FILE__));
    Str rootDir = prb_getParentDir(arena, srcDir);
    Str ezunicodeMainFilePath = prb_pathJoin(arena, rootDir, STR("ezunicode.h"));
    Str ezunicodeMainFileContent = readFile(arena, ezunicodeMainFilePath);

    StrBuilder* ezunicodeNewContentBuilder = beginStr(arena, 20 * prb_MEGABYTE);

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

        prb_StrScanner mainScanner = prb_createStrScanner(ezunicodeMainFileContent);
        insertBetween(&ezunicodeNewContentBuilder->gstr, &mainScanner, stbttheader, STR("#ifdef ezu_USE_STB_TRUETYPE"), STR("#endif  // ezu_USE_STB_TRUETYPE"));
        insertBetween(&ezunicodeNewContentBuilder->gstr, &mainScanner, stbttbody, STR("#ifdef ezu_USE_STB_TRUETYPE"), STR("#endif  // ezu_USE_STB_TRUETYPE"));

        prb_addStrSegment(&ezunicodeNewContentBuilder->gstr, "%.*s", LIT(mainScanner.afterMatch));
    }

    Str str = prb_endStr(&ezunicodeNewContentBuilder->gstr);
    assert(prb_writeEntireFile(arena, ezunicodeMainFilePath, str.ptr, str.len));

    return 0;
}
