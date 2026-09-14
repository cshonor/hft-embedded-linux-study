/* TLPI 第 03 章 §3.3 —— 标准 C 库 vs GNU C 库：怎么认出你用的是哪一个
 *
 * 三层身份要分开：
 *   ① C 语言标准（ISO C / C99 / C11…）—— 由 __STDC_VERSION__ 标出
 *   ② POSIX / SUS 规范            —— 由 _POSIX_VERSION 标出（来自 unistd.h）
 *   ③ 具体实现（glibc / musl / uClibc / bionic）—— glibc 会定义 __GLIBC__ 并给出
 *      gnu_get_libc_version()、confstr(_CS_GNU_LIBC_VERSION)
 *
 * 嵌入式/HFT 的用处：交叉编译时「板上跑的是 glibc 还是 musl」决定了一堆行为差异
 * （NSS、locale、malloc 实现、dlopen 语义），所以部署脚本里认一下版本很值。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_3 c3_3_glibc.c
 */
#define _GNU_SOURCE
#include <features.h>
#include <gnu/libc-version.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("=== ① 实现身份：这是 glibc 吗？ ===\n");
#ifdef __GLIBC__
    printf("  __GLIBC__       = %d\n", __GLIBC__);
    printf("  __GLIBC_MINOR__ = %d\n", __GLIBC_MINOR__);
#else
    printf("  未定义 __GLIBC__ → 不是 glibc（可能是 musl / uClibc / bionic）\n");
#endif

#ifdef __GLIBC_PREREQ
    printf("  __GLIBC_PREREQ(2, 34) = %d  (运行期/编译期都可用于条件编译)\n",
           __GLIBC_PREREQ(2, 34) ? 1 : 0);
#else
    printf("  未定义 __GLIBC_PREREQ\n");
#endif

    printf("\n=== ② 运行期问库自己（比宏更可信：宏只反映头文件版本） ===\n");
#ifdef __GLIBC__
    printf("  gnu_get_libc_version()          = %s\n", gnu_get_libc_version());
    printf("  gnu_get_libc_release()          = %s\n", gnu_get_libc_release());
#endif
    char cbuf[64] = {0};
    if (confstr(_CS_GNU_LIBC_VERSION, cbuf, sizeof cbuf) > 0) {
        printf("  confstr(_CS_GNU_LIBC_VERSION)   = %s\n", cbuf);
    } else {
        printf("  confstr(_CS_GNU_LIBC_VERSION)   不可用\n");
    }

    printf("\n=== ③ C 语言标准版本 ===\n");
#ifdef __STDC_VERSION__
    printf("  __STDC_VERSION__ = %ldL  (199901L=C99, 201112L=C11, 201710L=C17)\n",
           (long) __STDC_VERSION__);
#else
    printf("  __STDC_VERSION__ 未定义 → 编译器在 C89/C90 模式\n");
#endif
#ifdef __STDC__
    printf("  __STDC__         = %d\n", __STDC__);
#endif

    printf("\n=== ④ POSIX 规范版本（来自 unistd.h） ===\n");
#ifdef _POSIX_VERSION
    printf("  _POSIX_VERSION   = %ldL  (200809L = POSIX.1-2008)\n", (long) _POSIX_VERSION);
#else
    printf("  _POSIX_VERSION 未定义\n");
#endif

    printf("\n=== ⑤ 编译器身份 ===\n");
#ifdef __GNUC__
    printf("  __GNUC__         = %d  (gcc 主版本)\n", __GNUC__);
    printf("  __GNUC_MINOR__   = %d\n", __GNUC_MINOR__);
#endif
#ifdef __clang__
    printf("  __clang__        = %d.%d\n", __clang_major__, __clang_minor__);
#endif

    printf("\n=== ⑥ 同一个动作在两个实现下的差异（只列判据，不下结论）===\n");
    printf("  getpwnam() 走 NSS      → glibc 有；musl 只有简化实现、不支持 nsswitch\n");
    printf("  dlopen() 语义          → glibc 支持 RTLD_DEEPBIND 等 GNU 扩展；musl 不支持\n");
    printf("  %%m 格式符            → glibc 私有（printf 里输出 strerror(errno)），"
           "其它实现没有\n");
    printf("  __GLIBC__ / gnu_get_libc_version → glibc 独有，跨实现代码里要 #ifdef 包起来\n");
    return 0;
}
