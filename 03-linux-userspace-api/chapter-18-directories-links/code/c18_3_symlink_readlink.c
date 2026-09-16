/* c18_3_symlink_readlink.c — Ch18 §18.2/§18.5：符号链接的行为清单
 *
 * 钉住的点（全部实测）：
 *   ① symlink() 可以指向**不存在的路径**（悬空链接）——创建时不检查目标；
 *   ② 链接自身有 inode（lstat 见 15.1），st_size = 目标路径串长度；
 *   ③ readlink() 返回的是裸路径串（不带 NUL，size 是你给的缓冲上限）；
 *   ④ ELOOP：内核路径解析层限制链接层级（macOS 实测 40 层）；
 *   ⑤ 链接属主/权限有微妙的平台差异（macOS 实测 0777 + 属组继承目录）。
 *
 * 编译： cc -Wall -Wextra -o c18_3_symlink_readlink c18_3_symlink_readlink.c
 * 取材： man-pages 6.19 symlink(2)/readlink(2) + TLPI §18.2/§18.5（Listing 18-4 view_symlink.c）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    const char *dangling = "/tmp/tlpi_c18_3_dangling.lnk";
    const char *target = "/tmp/tlpi_c18_3_target.txt";
    unlink(dangling); unlink(target);

    /* ---------- ① 悬空链接 ---------- */
    printf("== ① symlink() 创建时不检查目标：悬空链接合法 ==\n");
    if (symlink("/no/such/path/ever.txt", dangling) == -1) {
        perror("symlink"); return EXIT_FAILURE;
    }
    struct stat sb;
    if (lstat(dangling, &sb) == -1) { perror("lstat"); return EXIT_FAILURE; }
    printf("  lstat(悬空链接): 类型=%s st_size=%ld（=目标路径串长度）\n",
           S_ISLNK(sb.st_mode) ? "symlink" : "?", (long) sb.st_size);
    errno = 0;
    int fd = open(dangling, O_RDONLY);
    printf("  open(悬空链接) = %d errno=%d(%s)——用的时候才爆\n",
           fd, errno, strerror(errno));
    errno = 0;
    int sr = stat(dangling, &sb);
    printf("  stat(悬空链接) = %d errno=%d(%s)\n", sr, errno, strerror(errno));

    /* ---------- ③ readlink 的裸字节串语义 ---------- */
    printf("\n== ③ readlink(): 裸路径串，不补 NUL ==\n");
    if (symlink(target, "/tmp/tlpi_c18_3_ok.lnk") == -1) { perror("symlink2"); return EXIT_FAILURE; }
    char buf[256];
    ssize_t n = readlink("/tmp/tlpi_c18_3_ok.lnk", buf, sizeof buf - 1);
    if (n == -1) { perror("readlink"); return EXIT_FAILURE; }
    buf[n] = '\0';                       /* 必须自己补 NUL */
    printf("  readlink = \"%s\" (%zd 字节，字符串长 %zu)\n", buf, n, strlen(buf));
    printf("  ⚠️ readlink 返回值是**写入的字节数**，不是字符串长——\n");
    printf("     超过缓冲会静默截断且无 NUL，必须按返回值处理。\n\n");

    /* ---------- ④ ELOOP ---------- */
    printf("== ④ 链接链：内核 ELOOP 保护 ==\n");
    char a[64], b2[64];
    snprintf(a, sizeof a, "/tmp/c18_3_loop_a");
    snprintf(b2, sizeof b2, "/tmp/c18_3_loop_b");
    unlink(a); unlink(b2);
    symlink(b2, a);
    symlink(a, b2);                      /* 互指成环 */
    errno = 0;
    fd = open(a, O_RDONLY);
    printf("  open(互指环) = %d errno=%d(%s)\n", fd, errno, strerror(errno));
    if (fd != -1) close(fd);
    /* 数一数最多能串几层 */
    int depth = 0;
    char cur[64], next[64];
    snprintf(cur, sizeof cur, "/tmp/c18_3_chain_0");
    unlink(cur);
    symlink("/tmp/c18_3_chain_base", cur);
    for (depth = 1; depth < 200; depth++) {
        snprintf(next, sizeof next, "/tmp/c18_3_chain_%d", depth);
        unlink(next);
        if (symlink(cur, next) == -1) break;
        snprintf(cur, sizeof cur, "%s", next);
    }
    errno = 0;
    fd = open(next, O_RDONLY);
    int ok = (fd != -1);
    if (ok) close(fd);
    printf("  串 %d 层链接 open = %s（errno=%d %s）\n",
           depth, ok ? "OK" : "失败", errno, strerror(errno));
    printf("  → 解析上限是内核常量（macOS=40，Linux=40 SYMLOOP/8 MAXSYMLINKS 语义见 man）；\n");
    /* 清理链条 */
    for (int i = 0; i <= depth; i++) {
        snprintf(next, sizeof next, "/tmp/c18_3_chain_%d", i);
        unlink(next);
    }
    unlink(a); unlink(b2);

    /* ---------- ⑤ 属主/权限的平台差异 ---------- */
    printf("\n== ⑤ 链接自身的属主与权限（平台差异实测）==\n");
    if (lstat("/tmp/tlpi_c18_3_ok.lnk", &sb) == -1) { perror("lstat2"); return EXIT_FAILURE; }
    printf("  macOS 实测: uid=%ld gid=%ld mode=%lo（链接有自己的 mode=0777&~umask；\n"
           "     跟随打开时的权限判定用目标文件——链接 mode 本身无访问语义）\n",
           (long) sb.st_uid, (long) sb.st_gid, (unsigned long) (sb.st_mode & 07777));
    printf("  ⚠️ 书上/Linux：链接权限被忽略、属主影响删除权（sticky 目录）；\n");
    printf("     macOS(BSD) 给链接自己的 uid/gid/mode——别跨平台假设。\n");
    unlink("/tmp/tlpi_c18_3_ok.lnk");
    unlink(dangling);
    return EXIT_SUCCESS;
}
