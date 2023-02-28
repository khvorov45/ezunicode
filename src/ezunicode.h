#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#ifndef ezu_HEADER_FILE
#define ezu_HEADER_FILE

// TODO(khvorov) Remove
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef ezu_PUBLICAPI
#define ezu_PUBLICAPI
#endif

#ifndef ezu_assert
#define ezu_assert(x)
#endif

#define ezu_min(a, b) (((a) < (b)) ? (a) : (b))
#define ezu_max(a, b) (((a) > (b)) ? (a) : (b))
#define ezu_arrayCount(x) (intptr_t)(sizeof((x)[0]) / sizeof(x))

//
// SECTION stb truetype (header)
//

typedef uint8_t  stbtt_uint8;
typedef int8_t   stbtt_int8;
typedef uint16_t stbtt_uint16;
typedef int16_t  stbtt_int16;
typedef uint32_t stbtt_uint32;
typedef int32_t  stbtt_int32;

#define STBTT_ifloor(x) ((int)floor(x))
#define STBTT_iceil(x) ((int)ceil(x))
#define STBTT_sqrt(x) sqrt(x)
#define STBTT_pow(x, y) pow(x, y)
#define STBTT_fmod(x, y) fmod(x, y)
#define STBTT_cos(x) cos(x)
#define STBTT_acos(x) acos(x)
#define STBTT_fabs(x) fabs(x)
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#define STBTT_assert(x) ezu_assert(x)
#define STBTT_strlen(x) strlen(x)
#define STBTT_memcpy memcpy
#define STBTT_memset memset

// @stbttheader

typedef struct ezu_Rect2i {
    intptr_t left;
    intptr_t top;
    intptr_t width;
    intptr_t height;
} ezu_Rect2i;

#define ezu_FONT_COUNT // @fontcount

typedef struct ezu_Context {
    stbtt_fontinfo stbttfonts[ezu_FONT_COUNT];
} ezu_Context;

//
// SECTION Core (header)
//

// @coreheader

#endif  // ezu_HEADER_FILE

#ifdef ezu_IMPLEMENTATION

// @fontdata

//
// SECTION Core (implementation)
//

ezu_PUBLICAPI ezu_Context
ezu_createContext(void) {
    ezu_Context ctx = {};
    for (intptr_t ind = 0; ind < ezu_FONT_COUNT; ind++) {
        ezu_assert(stbtt_InitFont(ctx.stbttfonts + ind, ezu_FontData_Array[ind], 0));
    }
    return ctx;
}

ezu_PUBLICAPI ezu_Rect2i
ezu_drawGlyphUtf32(ezu_Context* ctx, uint8_t* imageBuffer, intptr_t imageWidth, intptr_t imageHeight, uint32_t glyphUtf32) {
    intptr_t fontIndex = ezu_getFontIndexWithUtf32Glyph(glyphUtf32);
    // TODO(khvorov) What if the font isn't found?
    if (fontIndex == -1) {
        fontIndex = 0;
    }
    stbtt_fontinfo* font = ctx->stbttfonts + fontIndex;
    float           scale = stbtt_ScaleForPixelHeight(font, 200);
    int             x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(font, glyphUtf32, scale, scale, &x0, &y0, &x1, &y1);
    intptr_t glyphWidth = ezu_min(x1 - x0, imageWidth);
    intptr_t glyphHeight = ezu_min(y1 - y0, imageHeight);
    stbtt_MakeCodepointBitmap(font, (unsigned char*)imageBuffer, glyphWidth, glyphHeight, imageWidth, scale, scale, glyphUtf32);
    ezu_Rect2i result = {0, 0, glyphWidth, glyphHeight};
    return result;
}

ezu_PUBLICAPI intptr_t
ezu_getFontIndexWithUtf32Glyph(uint32_t glyphUtf32) {
    // @getfontindex
    return -1;
}

ezu_PUBLICAPI ezu_Rect2i
ezu_clipRectToRect(ezu_Rect2i rect, ezu_Rect2i clip) {
    intptr_t   left = ezu_max(rect.left, clip.left);
    intptr_t   top = ezu_max(rect.top, clip.top);
    intptr_t   right = ezu_min(rect.left + rect.width, clip.left + clip.width);
    intptr_t   bottom = ezu_min(rect.top + rect.height, clip.top + clip.height);
    intptr_t   width = right - left;
    intptr_t   height = bottom - top;
    ezu_Rect2i result = {left, top, width, height};
    return result;
}

//
// SECTION stb truetype (implementation)
//

// @stbttbody

#endif  // ezu_IMPLEMENTATION

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif
