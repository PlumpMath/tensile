#if __cplusplus
#define C_REVISION "c++"
#elif __OBJC__
#define C_REVISION "objc"
#elif __STDC_VERSION__ >= 201112L
#define C_REVISION "c11"
#elif __STDC_VERSION__ >= 199901L
#if defined(__STDC_UTF_32__)
#define C_REVISION "c99u"
#else
#define C_REVISION "c99"
#endif
#else
#define C_REVISION
#endif
#if defined(__clang__)
"clang" __clang_major__.__clang_minor__ C_REVISION
#elif defined(__GNUC__)
"gcc" __GNUC__.__GNUC_MINOR__ C_REVISION
#else
C_REVISION
#endif
