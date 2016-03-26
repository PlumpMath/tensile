#if defined(__amd64__)
#if defined(__ILP32__)
"amd64x32"
#else
"amd64"
#endif
#elif defined(__i386__)
#if defined(__i686__)
"i686"
#elif defined(__i586__)
"i586"
#elif defined(__i486__)
"i486"
#else
"i386"
#endif
#elif defined(__ia64__)
"ia64"
#elif defined(__powerpc64__)
#if defined(__BIG_ENDIAN__)
"ppc64"
#else
"ppc64el"
#endif
#elif defined(__powerpc__)
"ppc"
#elif defined(__aarch64__)
#if defined(__BIG_ENDIAN__)
"arm64eb"
#else
"arm64el"
#endif
#elif defined(__arm__)
#if defined(__SOFTFP__)
#define FP_SUFFIX
#else
#define FP_SUFFIX "+hf"
#endif
#if defined(__BIG_ENDIAN__)
"armeb" FP_SUFFIX
#else
"armel" FP_SUFFIX
#endif
#elif __mips == 64
#if defined(__MIPSEL__)
"mips64el"
#else
"mips64eb"
#endif
#elif defined(__mips__)
#if defined(__MIPSEL__)
"mipsel"
#else
"mipseb"
#endif
#elif defined(__sparc64__)
"sparc64"
#elif defined(__sparc__)
"sparc"
#elif defined(__s390x__)
"s390x"
#elif defined(__s390__)
"s390"
#else
"unknown"
#endif
