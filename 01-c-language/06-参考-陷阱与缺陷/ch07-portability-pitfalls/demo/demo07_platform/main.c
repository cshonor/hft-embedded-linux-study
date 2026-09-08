#include <stdio.h>

int main(void)
{
    printf("pointer size: %zu bytes\n", sizeof(void *));

#if defined(__linux__)
    printf("OS: __linux__ (POSIX APIs like mmap/fork available)\n");
#elif defined(_WIN32)
    printf("OS: _WIN32 (use Win32 API, not fork/mmap)\n");
#else
    printf("OS: other — wrap platform-specific code\n");
#endif

#if defined(__x86_64__)
    printf("arch: __x86_64__\n");
#elif defined(__aarch64__)
    printf("arch: __aarch64__\n");
#elif defined(__i386__)
    printf("arch: __i386__\n");
#else
    printf("arch: (see compiler predefined macros)\n");
#endif

#if defined(__GNUC__)
    printf("compiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif

    puts("centralize checks in platform.h — do not scatter raw macros");

    return 0;
}
