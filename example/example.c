#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#if defined(WIN32) || defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif (defined(linux) || defined(__linux) || defined(__linux__))
#define PLATFORM_LINUX 1
#else
#error unrecognized platform
#endif

// clang-format off
#define function static
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define arrayCount(arr) (isize)(sizeof(arr) / sizeof(arr[0]))
#define arenaAllocArray(arena, type, len) (type*)arenaAllocAndZero(arena, (len) * (isize)sizeof(type), alignof(type))
#define arenaAllocStruct(arena, type) (type*)arenaAllocAndZero(arena, sizeof(type), alignof(type))
#define isPowerOf2(x) (((x) > 0) && (((x) & ((x)-1)) == 0))
#define unused(x) ((x) = (x))
#define Byte 1
#define Kilobyte 1024 * Byte
#define Megabyte 1024 * Kilobyte
#define Gigabyte 1024 * Megabyte
#define STR(x) (Str) {x, strlen(x)}
#define debugbreak() __builtin_debugtrap()
#define assert(condition) do { if (condition) {} else { debugbreak(); } } while (0)
// clang-format on

#define ezu_IMPLEMENTATION
#define ezu_PUBLICAPI static
#define ezu_assert(x) assert(x)
#include "../ezunicode.h"

typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef intptr_t  isize;
typedef uint8_t   u8;
typedef uint64_t  u64;
typedef uintptr_t usize;
typedef float     f32;

//
// SECTION Memory
//

// function void
// memcpy(void* dest, void* src, isize len) {
//     for (isize ind = 0; ind < len; ind++) {
//         ((u8*)dest)[ind] = ((u8*)src)[ind];
//     }
// }

// function bool
// memeq(const void* ptr1, const void* ptr2, isize len) {
//     bool result = true;
//     for (isize ind = 0; ind < len; ind++) {
//         if (((u8*)ptr1)[ind] != ((u8*)ptr2)[ind]) {
//             result = false;
//             break;
//         }
//     }
//     return result;
// }

typedef struct Arena {
    void* base;
    isize size;
    isize used;
    isize tempCount;
} Arena;

typedef struct TempMemory {
    Arena* arena;
    isize  usedAtBegin;
    isize  tempCountAtBegin;
} TempMemory;

function isize
getOffsetForAlignment(void* ptr, isize align) {
    assert(isPowerOf2(align));
    usize ptrAligned = (usize)((u8*)ptr + (align - 1)) & (usize)(~(align - 1));
    assert(ptrAligned >= (usize)ptr);
    isize diff = (isize)(ptrAligned - (usize)ptr);
    assert(diff < align && diff >= 0);
    return diff;
}

function void*
arenaFreePtr(Arena* arena) {
    void* result = (u8*)arena->base + arena->used;
    return result;
}

function isize
arenaFreeSize(Arena* arena) {
    isize result = arena->size - arena->used;
    return result;
}

function void
arenaChangeUsed(Arena* arena, isize byteDelta) {
    assert(arenaFreeSize(arena) >= byteDelta);
    arena->used += byteDelta;
}

function void
arenaAlignFreePtr(Arena* arena, isize align) {
    isize offset = getOffsetForAlignment(arenaFreePtr(arena), align);
    arenaChangeUsed(arena, offset);
}

function void*
arenaAllocAndZero(Arena* arena, isize size, isize align) {
    arenaAlignFreePtr(arena, align);
    void* result = arenaFreePtr(arena);
    arenaChangeUsed(arena, size);
    for (isize ind = 0; ind < size; ind++) {
        ((u8*)arena->base)[arena->used + ind] = 0;
    }
    return result;
}

// function Arena
// createArenaFromArena(Arena* parent, isize bytes) {
//     Arena arena = {
//         .base = arenaFreePtr(parent),
//         .size = bytes,
//         .used = 0,
//         .tempCount = 0,
//     };
//     arenaChangeUsed(parent, bytes);
//     return arena;
// }

// function TempMemory
// beginTempMemory(Arena* arena) {
//     TempMemory temp = {.arena = arena, .usedAtBegin = arena->used, .tempCountAtBegin = arena->tempCount};
//     arena->tempCount += 1;
//     return temp;
// }

// function void
// endTempMemory(TempMemory temp) {
//     assert(temp.arena->tempCount == temp.tempCountAtBegin + 1);
//     temp.arena->used = temp.usedAtBegin;
//     temp.arena->tempCount -= 1;
// }

typedef struct Bytes {
    u8*   data;
    isize len;
} Bytes;

//
// SECTION Entry
//

#if PLATFORM_LINUX
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

#undef function
#include <X11/Xlib.h>

int
main() {
    Arena  arena_ = {};
    Arena* arena = &arena_;
    {
        arena->size = 1 * Gigabyte;
        arena->base = mmap(0, arena->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        assert(arena->base != MAP_FAILED);
    }

    Display* x11display = XOpenDisplay(NULL);
    assert(x11display);

    isize windowWidth = 1000;
    isize windowHeight = 1000;

    XImage* x11image = XCreateImage(x11display, DefaultVisual(x11display, 0), 24, ZPixmap, 0, 0, windowWidth, windowHeight, 32, 0);
    u32*    imageBuffer = arenaAllocArray(arena, u32, windowWidth * windowHeight);
    x11image->data = (char*)imageBuffer;

    isize glyphAlphaBufferWidth = 500;
    isize glyphAlphaBufferHeight = 500;
    u8*   glyphAlphaBuffer = arenaAllocArray(arena, u8, glyphAlphaBufferWidth * glyphAlphaBufferHeight);

    Window x11window = XCreateWindow(
        x11display,
        DefaultRootWindow(x11display),
        0,
        0,
        windowWidth,
        windowHeight,
        0,
        0,
        InputOutput,
        CopyFromParent,
        0,
        0
    );

    Atom wmDeleteWindow = XInternAtom(x11display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x11display, x11window, &wmDeleteWindow, 1);

    XSelectInput(
        x11display,
        x11window,
        SubstructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            | KeyPressMask | KeyReleaseMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask
            | ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask
    );

    XMapWindow(x11display, x11window);

    ezu_Context ezuctx = ezu_createContext();

    isize cursorX = 0;
    isize cursorY = 0;
    u32   glyphToDraw = 12390;
    for (;;) {
        isize processedEventCount = 0;
        for (;;) {
            XEvent event = {};
            if (XPending(x11display) || (processedEventCount == 0)) {
                XNextEvent(x11display, &event);
            } else {
                break;
            }

            if (!XFilterEvent(&event, x11window)) {
                processedEventCount += 1;

                switch (event.type) {
                    case ClientMessage: _exit(0); break;
                    case MotionNotify: {
                        cursorX = event.xmotion.x;
                        cursorY = event.xmotion.y;
                    } break;
                    case ButtonPress: glyphToDraw++; break;
                }
            }
        }

        memset(imageBuffer, 0, windowWidth * windowHeight * sizeof(*imageBuffer));

        isize glyphImageLeft = cursorX;
        isize glyphImageTop = cursorY;

        ezu_Rect2i glyphAlphaBufferRect = {};
        {
            ezu_Rect2i full = ezu_drawGlyphUtf32(&ezuctx, glyphAlphaBuffer, glyphAlphaBufferWidth, glyphAlphaBufferHeight, ezu_FontID_NotoSansJPRegular, glyphToDraw);
            ezu_Rect2i image = {-glyphImageLeft, -glyphImageTop, windowWidth, windowHeight};
            glyphAlphaBufferRect = ezu_clipRectToRect(full, image);
        }
        for (isize glyphY = glyphAlphaBufferRect.top; glyphY < glyphAlphaBufferRect.top + glyphAlphaBufferRect.height; glyphY++) {
            for (isize glyphX = glyphAlphaBufferRect.left; glyphX < glyphAlphaBufferRect.left + glyphAlphaBufferRect.width; glyphX++) {
                isize glyphIndex = glyphY * glyphAlphaBufferWidth + glyphX;
                assert(glyphIndex >= 0 && glyphIndex < glyphAlphaBufferWidth * glyphAlphaBufferHeight);
                u8 glyphAlpha = glyphAlphaBuffer[glyphIndex];

                isize imageIndex = (glyphImageTop + glyphY) * windowWidth + (glyphImageLeft + glyphX);
                assert(imageIndex >= 0 && imageIndex < windowWidth * windowHeight);
                imageBuffer[imageIndex] = (glyphAlpha << 16) | (glyphAlpha << 8) | (glyphAlpha);
            }
        }

        XPutImage(
            x11display,
            x11window,
            DefaultGC(x11display, 0),
            x11image,
            0,
            0,
            0,
            0,
            windowWidth,
            windowHeight
        );
    }

    return 0;
}

#elif PLATFORM_WINDOWS
#error unimplemented
#endif
