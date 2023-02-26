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

typedef struct ezu_Rect2i {
    intptr_t left;
    intptr_t top;
    intptr_t width;
    intptr_t height;
} ezu_Rect2i;

//
// SECTION Core (header)
//

// @coreheader

//
// SECTION stb truetype (header)
//

#ifdef ezu_USE_STB_TRUETYPE
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
#endif  // ezu_USE_STB_TRUETYPE

#endif  // ezu_HEADER_FILE

#ifdef ezu_IMPLEMENTATION

#ifdef ezu_INCLUDE_FONT_DATA
// @fontdata
#endif  // ezu_INCLUDE_FONT_DATA

//
// SECTION Core (implementation)
//

ezu_PUBLICAPI ezu_Rect2i
ezu_drawUnicode(uint8_t* imageBuffer, intptr_t imageWidth, intptr_t imageHeight) {
    stbtt_fontinfo font = {};
    ezu_assert(stbtt_InitFont(&font, ezu_FontData_NotoSansRegular, 0));
    float scale = stbtt_ScaleForPixelHeight(&font, 200);
    int   x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&font, 'a', scale, scale, &x0, &y0, &x1, &y1);
    intptr_t glyphWidth = ezu_min(x1 - x0, imageWidth);
    intptr_t glyphHeight = ezu_min(y1 - y0, imageHeight);
    stbtt_MakeCodepointBitmap(&font, (unsigned char*)imageBuffer, glyphWidth, glyphHeight, imageWidth, scale, scale, 'a');
    ezu_Rect2i result = {0, 0, glyphWidth, glyphHeight};
    return result;
}

//
// SECTION stb truetype (implementation)
//

#ifdef ezu_USE_STB_TRUETYPE
// @stbttbody
#endif  // ezu_USE_STB_TRUETYPE

#endif  // ezu_IMPLEMENTATION

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif
