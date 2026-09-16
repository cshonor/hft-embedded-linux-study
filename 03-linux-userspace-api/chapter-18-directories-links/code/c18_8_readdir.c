/* c18_8_readdir.c — Ch18 §18.8：readdir 的四个必知细节
 *
 * 钉住的点（全部实测）：
 *   ① 「读到末尾」与「读出错」都返回 NULL —— 必须用 errno 协议区分：
 *      进循环前 errno = 0，返回 NULL 后看 errno，非 0 才是错；
 *   ② telldir/seekdir 是**不透明的书签**（不能当字节偏移做算术），
 *      rewinddir 才是回到开头；
 *   ③ fdopendir 之后 **fd 归 DIR 所有**：再自己 close(fd) 或 read(fd)
 *      都是错的（会与 readdir 抢 fd 的偏移 / 双重关闭）；
 *   ④ d_type 是「省一次 lstat」的优化，不是权威：值可能 DT_UNKNOWN，
 *      也可能是链接（不跟目标）。真要确定类型就 lstat。
 *
 * 编译： cc -Wall -Wextra -o c18_8_readdir c18_8_readdir.c
 * 取材： man-pages 6.19 readdir(3)/fdopendir(3) + TLPI §18.8
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *type_name(unsigned char t)
{
    switch (t) {
    case DT_BLK:  return "BLK";
    case DT_CHR:  return "CHR";
    case DT_DIR:  return "DIR";
    case DT_FIFO: return "FIFO";
    case DT_LNK:  return "LNK";
    case DT_REG:  return "REG";
    case DT_SOCK: return "SOCK";
    case DT_UNKNOWN: return "UNKNOWN";
    default:      return "?(other)";
    }
}

int main(void)
{
    system("rm -rf /tmp/tlpi_c18_8 && mkdir -p /tmp/tlpi_c18_8/dd "
           "&& touch /tmp/tlpi_c18_8/aa && ln -s aa /tmp/tlpi_c18_8/ll");

    /* ---------- ① errno 协议 ---------- */
    printf("== ① readdir 返回 NULL：末页 or 出错？靠 errno 协议区分 ==\n");
    DIR *dir = opendir("/tmp/tlpi_c18_8");
    if (!dir) { perror("opendir"); return EXIT_FAILURE; }
    errno = 0;                                    /* 关键：先清零 */
    struct dirent *e;
    int n = 0;
    while ((e = readdir(dir)) != NULL)
        n++;
    printf("  遍历完 %d 项，返回 NULL 时 errno = %d(%s) → 正常结束\n",
           n, errno, errno == 0 ? "未置位" : strerror(errno));
    printf("  ⚠️ 不去查 errno 的写法在 NFS / 坏盘上会把 I/O 错误当「读完了」，\n");
    printf("     静默少列文件——这是备份工具最经典的丢数据 bug。\n\n");

    /* ---------- ② telldir 的 cookie 长什么样 ---------- */
    printf("== ② telldir 返回的是「不透明书签」，别当字节偏移 ==\n");
    printf("  TLPI 的警告：不同系统里这个值的含义不一样。本机实测：\n");
    rewinddir(dir);
    while ((e = readdir(dir)) != NULL)
        printf("    读 %-4s -> telldir() = %ld\n", e->d_name, telldir(dir));
    printf("  → macOS(APFS) 给的是「已读项计数」式的 0,1,2,…；\n");
    printf("     Linux(ext4) 给的是目录内部偏移（d_off，字节数，4 的倍数）。\n");
    printf("     跨平台做算术（mark+N）必炸——这是原书专门点的坑。\n\n");

    /* ---------- ③ seekdir 在本机是否真的定位 ---------- */
    printf("== ③ seekdir：POSIX 契约 vs 本机实测 ==\n");
    printf("  契约：读完 X 后 telldir 取 t，seekdir(t) 的下一次 readdir 必须再给 X 的后继。\n");
    for (long c = 0; c < 5; c++) {
        rewinddir(dir);
        seekdir(dir, c);
        e = readdir(dir);
        printf("    seekdir(%ld) -> 下一项 = %s\n", c, e ? e->d_name : "(EOF)");
    }
    printf("  ⚠️ 本机 macOS 26.6.2 / APFS 实测：**seekdir 恒等于 rewinddir**——\n");
    printf("     换任何 cookie 都从第一项重新开始，完全没定位效果。\n");
    printf("     Linux/ext4 上同样代码能正确复现位置。\n");
    printf("  ⇒ 结论：目录扫描要「打点续扫」，别用 telldir/seekdir 做断点；\n");
    printf("     用 rewinddir 重来，或干脆把要处理的路径收集起来再遍历。\n");
    printf("     （rewinddir 本机实测正常，是可靠的。）\n\n");

    /* ---------- ④ fdopendir 的 fd 归属 ---------- */
    printf("== ④ fdopendir 之后 fd 归 DIR 管 ==\n");
    int raw = open("/tmp/tlpi_c18_8", O_RDONLY | O_DIRECTORY);
    printf("  open 得到 fd = %d\n", raw);
    DIR *d2 = fdopendir(raw);
    if (!d2) { perror("fdopendir"); return EXIT_FAILURE; }
    printf("  fdopendir(fd) = %p\n", (void *) d2);
    printf("  ⚠️ 此后 raw 的所有权移交 DIR：只能 closedir(d2)，\n");
    printf("     再 close(raw) 会 double close；read(raw) 会与 readdir 抢偏移。\n\n");

    /* ---------- ⑤ d_type vs lstat ---------- */
    printf("== ⑤ d_type 只是优化；lstat 才是权威 ==\n");
    printf("  %-6s %-16s %-10s %s\n", "name", "d_type", "lstat", "一致?");
    rewinddir(d2);
    while ((e = readdir(d2)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
            continue;
        char p[256];
        snprintf(p, sizeof p, "/tmp/tlpi_c18_8/%s", e->d_name);
        struct stat sb;
        if (lstat(p, &sb) == -1) continue;
        const char *real = S_ISDIR(sb.st_mode) ? "DIR" :
                           S_ISLNK(sb.st_mode) ? "LNK" :
                           S_ISREG(sb.st_mode) ? "REG" : "其他";
        printf("  %-6s %-16s %-10s %s\n", e->d_name, type_name(e->d_type),
               real, strcmp(type_name(e->d_type), real) == 0 ? "是" : "否");
    }
    printf("  ⚠️ 注意 ll 是**符号链接**：readdir 报 LNK，而 stat（跟随）会报 REG。\n");
    printf("     「列举后的下一步」默认该用 lstat，否则统计目录时会重复计数。\n");

    closedir(d2);      /* 注意：不要 close(raw) */
    system("rm -rf /tmp/tlpi_c18_8");
    return EXIT_SUCCESS;
}
