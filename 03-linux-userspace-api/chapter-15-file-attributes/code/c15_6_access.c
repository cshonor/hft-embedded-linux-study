/* c15_6_access.c — Ch15 §15.4.3/15.4.4：权限判定算法与 access()
 *
 * TLPI §15.4.3 的判定算法一句话：按「你是谁」三选一，选中的那一个
 * 说了算，**不叠加**：
 *   euid == 文件属主      → 只看属主三元组
 *   egid/补充组 ∋ 属组    → 只看属组三元组
 *   都不是                → 只看其他三元组
 * 「属主没有 r，但其他有 r」→ 属主自己也被拒（15-1 习题 a 的机制根源）。
 *
 * §15.4.4 的 access() 用的是 **real** uid/gid，专为 SUID 程序设计；
 * 本 demo 以非特权身份验证：
 *   ① 属主位被剥光时，属主自己读不了（哪怕 group/other 全开）；
 *   ② access() 对 mode 000 文件的 F_OK / R_OK / W_OK / X_OK 逐一回答；
 *   ③ faccessat(AT_EACCESS) 用有效 ID 判——普通进程里与 access()
 *      等价，SUID 程序里才是分水岭（本机无特权，标注说明）。
 *
 * 编译： cc -Wall -Wextra -o c15_6_access c15_6_access.c
 * 取材： man-pages 6.19 access(2) / faccessat(2)（NOTES: real vs effective）
 *       TLPI §15.4.3/15.4.4（算法与 access 用途）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void probe(const char *path, int mode, const char *name)
{
    errno = 0;
    int r = access(path, mode);
    printf("  access(%-28s, %-5s) = %2d  %s\n", path, name, r,
           r == 0 ? "允许" : (errno ? strerror(errno) : "拒绝"));
}

int main(void)
{
    const char *path = "/tmp/tlpi_c15_6.txt";
    unlink(path);

    printf("== ⓪ 身份：ruid=%ld euid=%ld rgid=%ld egid=%ld（普通进程两者一致）==\n\n",
           (long) getuid(), (long) geteuid(), (long) getgid(), (long) getegid());

    /* ---------- ① 属主被剥光：三选一、不叠加 ---------- */
    printf("== ① mode=0077：属主位=---，属主自己也被拒（不叠加！）==\n");
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0077);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (write(fd, "x", 1) != 1) { perror("write"); return EXIT_FAILURE; }
    close(fd);
    if (chmod(path, 0077) == -1) { perror("chmod"); return EXIT_FAILURE; }
    /* ↑ 创建时给的 0077 会被进程 umask 剪掉一部分（本机剪成 0055），
     * 所以显式 chmod 一次，保证实验的前提「属主位=---, 组/其他=rwx」成立 */
    struct stat sb;
    if (stat(path, &sb) == -1) { perror("stat"); return EXIT_FAILURE; }
    printf("  文件属主 = %ld（就是我们），mode=%04lo（0077；创建时被 umask 剪成 0055 的教训见 15.4.6）\n",
           (long) sb.st_uid, (unsigned long) (sb.st_mode & 07777));
    probe(path, R_OK, "R_OK");
    printf("  → 「其他」位明明有 r，但属主判定走属主三元组：---，拒。\n");
    printf("     权限判定是 switch 不是 if 连判——这条是 15.4.3 的灵魂。\n\n");

    /* ---------- ② access() 四连测 ---------- */
    printf("== ② mode=0000 时 access() 四连测 ==\n");
    if (chmod(path, 0000) == -1) { perror("chmod"); return EXIT_FAILURE; }
    probe(path, F_OK, "F_OK");
    probe(path, R_OK, "R_OK");
    probe(path, W_OK, "W_OK");
    probe(path, X_OK, "X_OK");
    printf("  → F_OK 只问「存在」，其余按属主三元组 0 位全拒。\n\n");

    /* ---------- ③ 恢复后 access 与实际 open 一致 ---------- */
    printf("== ③ mode=0644 时 access 与 open 对拍 ==\n");
    if (chmod(path, 0644) == -1) { perror("chmod2"); return EXIT_FAILURE; }
    probe(path, R_OK, "R_OK");
    probe(path, W_OK, "W_OK");
    errno = 0;
    int fd2 = open(path, O_RDONLY);
    printf("  open(O_RDONLY) 实际 = %s\n", fd2 == -1 ? strerror(errno) : "OK");
    if (fd2 != -1) close(fd2);

    printf("\n== ④ access() 存在的意义（SUID 场景，本机无法实测，机制说明）==\n");
    printf("  SUID 程序的 euid=0：open() 按有效 ID 判，root 全通；\n");
    printf("  access() 按 real ID 判，能回答「**发起者**本人到底有没有权限」。\n");
    printf("  faccessat(AT_EACCESS) 则显式选「按有效 ID」——同一接口两种语义。\n");
    printf("  ⚠️ access() 有 TOCTOU 缺陷：查完到真正 open 之间权限可被换掉；\n");
    printf("     需要原子判定时用 open+返回码（或 seteuid 切换后再 open）。\n");

    unlink(path);
    return EXIT_SUCCESS;
}
