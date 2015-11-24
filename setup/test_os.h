#if defined(__ANDROID__)
"android"
#elif defined(__uClinux__)
"uclinux"
#elif defined(__gnu_linux__)
"gnulinux"
#elif defined(__linux__)
"otherlinux"
#elif defined(__FreeBSD__)
"freebsd"
#elif defined(__FreeBSD_kernel__) && defined(__GLIBC__)
"gnufreebsd"
#elif defined(__OpenBSD__)
"openbsd"
#elif defined(__NetBSD__)
"netbsd"
#elif defined(__sun__)
"solaris"
#elif defined(__gnu_hurd__)
"gnuhurd"
#elif defined(_AIX)
"aix"
#elif defined(__APPLE__)
"darwin"
#elif defined(__CYGWIN__)
"cygwin"
#elif defined(_WIN64)
"win64"
#elif defined(_WIN32)
"win32"
#elif defined(__rtems__)
"rtems"
#elif defined(__GO32__)
"djgpp"
#elif defined(__unix__)
"posix"
#else
"unknown"
#endif
