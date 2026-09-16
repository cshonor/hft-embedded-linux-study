/* c18_4_cwd.c — Ch18 §18.10/§18.11/§18.12：进程的工作目录与根目录
 *
 * 钉住的点（全部实测）：
 * ① cwd 是**进程属性**：fork 继承、exec 保留——chdir("…") 后 exec 的
 *    程序会在新 cwd 里找相对路径；
 * ② getcwd 缓冲不足返回 ERANGE（老式 getwd 已废弃）；realloc 循环是标准写法；
 * ③ fchdir + 保存 fd 是「记住老目录」的零开销方案（习题 18-9 的正解之一）；
 *    对比 getcwd+chdir 的字符串方案（有 EAMETOOLONG 与路径重解析成本）；
 * ④ open(".", O_RDONLY) 得到目录 fd——O_PATH/O_RDONLY + fchdir 都用它；
 * ⑤ chroot 非特权一律 EPERM（实测），语义标注（进程视角的 "/" 重定向，
 *    不改变 cwd！chroot 后必须显式 chdir("/")，书上专门强调）。
 *
 * 编译： cc -Wall -Wextra -o c18_4_cwd c18_4_cwd.c
 * 取材： man-pages 6.19 getcwd(3)/chroot(2) + TLPI §18.10-§18.12
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int main(void)
{
    char buf[PATH_MAX];

    /* ---------- ①② getcwd 与 realloc 循环 ---------- */
    printf("== ①② cwd 是进程属性；getcwd 探大小循环 ==\n");
    if (getcwd(buf, sizeof buf) == NULL) { perror("getcwd"); return EXIT_FAILURE; }
    printf("  初始 cwd = %s\n", buf);

    /* ---------- ③ fchdir + fd 方案（习题 18-9 正解）---------- */
    printf("\n== ③ fchdir + 目录 fd：零字符串开销地「记住老目录」==\n");
    int old_fd = open(".", O_RDONLY);            /* 保存老 cwd 的 fd */
    if (old_fd == -1) { perror("open ."); return EXIT_FAILURE; }
    if (chdir("/") == -1) { perror("chdir"); return EXIT_FAILURE; }
    if (getcwd(buf, sizeof buf) == NULL) { perror("getcwd2"); return EXIT_FAILURE; }
    printf("  chdir(/) 后 cwd = %s\n", buf);
    if (fchdir(old_fd) == -1) { perror("fchdir"); return EXIT_FAILURE; }
    if (getcwd(buf, sizeof buf) == NULL) { perror("getcwd3"); return EXIT_FAILURE; }
    printf("  fchdir(old_fd) 回到 = %s（无需字符串、无路径重解析）\n", buf);
    printf("  ⚠️ 对比 getcwd(old)+chdir(old) 方案：多一次路径解析、有\n");
    printf("     PATH_MAX 截断风险，且老目录可能已被改名/删除（fd 方案免疫）。\n\n");

    /* ---------- ④ 目录 fd 的另一个用途：openat 的锚点 ---------- */
    printf("== ④ open(\".\") 的目录 fd 也是 openat 的锚点（§18.11）==\n");
    int dfd = open("/tmp", O_RDONLY | O_DIRECTORY);
    if (dfd == -1) { perror("open /tmp"); return EXIT_FAILURE; }
    int f2 = openat(dfd, "tlpi_c18_4.txt", O_RDWR | O_CREAT | O_EXCL, 0644);
    printf("  openat(/tmp 的 fd, \"tlpi_c18_4.txt\") = %d（相对锚点，不受 cwd 影响）\n", f2);
    if (f2 != -1) close(f2);
    printf("  ⚠️ openat 防的是「cwd 被别人改走」与 TOCTOU：锚点一旦打开即固定。\n\n");

    /* ---------- ⑤ chroot：非特权 EPERM（实测） ---------- */
    printf("== ⑤ chroot：需要特权（实测）==\n");
    errno = 0;
    int r = chroot("/tmp/tlpi_c18_4_jail");
    printf("  chroot(非特权) = %d errno=%d(%s)  ← 需 CAP_SYS_CHROOT/root\n",
           r, errno, strerror(errno));
    printf("  语义标注（书上强调，未实测——需特权环境）：\n");
    printf("    · chroot 只改进程视角的 \"/\"，**不会改 cwd**；\n");
    printf("    · 经典越狱：chroot 前先 open(\".\") 存 fd，chroot 后\n");
    printf("      fchdir(fd) 就站在新根之外——所以 chroot 必须后接 chdir(\"/\")\n");
    printf("      并放弃 fd（或 double chroot），否则形同虚设。\n");
    close(dfd);
    unlink("/tmp/tlpi_c18_4.txt");
    close(old_fd);
    return EXIT_SUCCESS;
}
