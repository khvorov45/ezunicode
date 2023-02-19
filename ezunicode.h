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

#ifndef ezu_PUBLICAPI
#define ezu_PUBLICAPI
#endif

ezu_PUBLICAPI void ezu_drawUnicode(void);

#endif  // ezu_HEADER_FILE

#ifdef ezu_IMPLEMENTATION

ezu_PUBLICAPI void
ezu_drawUnicode(void) {
}

#endif  // ezu_IMPLEMENTATION

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif
