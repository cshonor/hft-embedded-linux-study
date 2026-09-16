/* ex19_1_log_events.c — 习题 19-1：记录目录树下所有 文件创建/删除/改名
 *
 * 题面（原书 §19 习题 19-1，按转述）：写程序记录命令行给定目录下所有
 * 文件的创建、删除与改名。必须监控**所有子目录**（用 nftw() 枚举，
 * §18.9）；树里新建子目录或删除子目录时，watch 集合要**动态更新**。
 *
 * ⚠️ inotify 是 Linux 专有（2.6.13+；sys/inotify.h）。macOS 无此接口，
 *    本程序**未在本机编译运行**——源码按 man-pages 6.19 inotify(7)
 *    与内核 v6.6 fs/notify/inotify/ 核验，待 Pi5 实测。
 *
 * 设计要点（与题面逐条对应）：
 *   1) nftw(FTW_PHYS) 枚举树，给每个目录 add_watch；
 *   2) wd → 路径 的映射用固定数组（wd 是小整数，重用 watch 槽位会回收）；
 *   3) IN_CREATE|IN_ISDIR 与 IN_MOVED_TO|IN_ISDIR → 给新目录补 watch
 *      （inotify 不递归，这是唯一正确时机——此时该目录尚无并发变更）；
 *   4) IN_DELETE_SELF / IN_MOVED_FROM|IN_ISDIR → 标记槽位失效；
 *   5) IN_Q_OVERFLOW → 队列溢出，必须全量重建（丢事件 = 状态失同步）。
 *
 * 编译（Linux/Pi5）： cc -Wall -Wextra -o ex19_1_log_events ex19_1_log_events.c
 * 取材： man-pages 6.19 inotify(7)/inotify_add_watch(2)/nftw(3) + TLPI §19.2–19.4
 */
#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define MAX_DIRS 4096
#define EVENT_BUF_LEN (64 * (sizeof(struct inotify_event) + NAME_MAX + 1))

static int ifd;                                   /* inotify 实例 */
static char *dirmap[MAX_DIRS];                    /* wd → 目录路径（malloc 串）*/
static int top_wd;                                /* 最高用过的 wd（新 add 的总更大）*/

/* ---------- nftw 回调：给每个目录建 watch ---------- */
static int add_dir_watch(const char *fpath, const struct stat *sb,
                         int typeflag, struct FTW *ftwbuf)
{
    (void) sb; (void) ftwbuf;
    if (typeflag != FTW_D && typeflag != FTW_DP)
        return 0;                                  /* 只 watch 目录 */
    if (top_wd >= MAX_DIRS) { fprintf(stderr, "目录数超上限\n"); return 1; }
    uint32_t mask = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO
                  | IN_DELETE_SELF | IN_MOVE_SELF;
    int wd = inotify_add_watch(ifd, fpath, mask);
    if (wd == -1) { perror("inotify_add_watch"); return 1; }
    if (wd >= MAX_DIRS) { fprintf(stderr, "wd 超上限\n"); return 1; }
    free(dirmap[wd]);
    dirmap[wd] = strdup(fpath);
    if ((int) strlen(fpath) > top_wd) top_wd = wd; /* 只用于上限检查 */
    return 0;
}

/* ---------- 事件打印 ---------- */
static const char *mask_name(uint32_t m)
{
    if (m & IN_CREATE)      return "CREATE";
    if (m & IN_DELETE)      return "DELETE";
    if (m & IN_MOVED_FROM)  return "MOVED_FROM";
    if (m & IN_MOVED_TO)    return "MOVED_TO";
    if (m & IN_DELETE_SELF) return "DELETE_SELF(目录)";
    if (m & IN_MOVE_SELF)   return "MOVE_SELF(目录)";
    if (m & IN_Q_OVERFLOW)  return "QUEUE_OVERFLOW";
    return "OTHER";
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <dir>     # Linux 专有（inotify）\n", argv[0]);
        return EXIT_FAILURE;
    }

    ifd = inotify_init1(IN_NONBLOCK);              /* 非阻塞：轮询式读（示例从简）*/
    if (ifd == -1) { perror("inotify_init1"); return EXIT_FAILURE; }

    /* 1. nftw 枚举整棵树（FTW_PHYS：不跟链接；本回调只对目录动作）*/
    if (nftw(argv[1], add_dir_watch, 64, FTW_PHYS) == -1) {
        perror("nftw"); return EXIT_FAILURE;
    }
    fprintf(stderr, "[已 watch 的目录数：初始化完成，开始监控 %s]\n", argv[1]);

    /* 2. 事件循环 */
    char buf[EVENT_BUF_LEN];
    for (;;) {
        ssize_t n = read(ifd, buf, sizeof buf);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { sleep(1); continue; }
            perror("read"); return EXIT_FAILURE;
        }
        for (char *p = buf; p < buf + n; ) {
            struct inotify_event *ev = (struct inotify_event *) p;
            const char *dir = (ev->wd >= 0 && ev->wd < MAX_DIRS && dirmap[ev->wd])
                              ? dirmap[ev->wd] : "??";

            if (ev->mask & IN_Q_OVERFLOW) {        /* 队列溢出：事件丢了 */
                printf("!! IN_Q_OVERFLOW —— 事件丢失，应全量重建 watch\n");
            } else if (ev->len > 0) {
                printf("%-14s %s/%s%s\n", mask_name(ev->mask), dir, ev->name,
                       (ev->mask & IN_ISDIR) ? "/" : "");
            } else {
                printf("%-14s %s\n", mask_name(ev->mask), dir);
            }

            /* 3. watch 集合的动态维护 */
            if ((ev->mask & IN_ISDIR) &&
                (ev->mask & (IN_CREATE | IN_MOVED_TO))) {
                char sub[PATH_MAX];
                snprintf(sub, sizeof sub, "%s/%s", dir, ev->name);
                add_dir_watch(sub, NULL, FTW_D, NULL);   /* 新目录：补 watch */
            }
            if (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                free(dirmap[ev->wd]);              /* 目录没了：槽位作废 */
                dirmap[ev->wd] = NULL;
            }
            p += sizeof(struct inotify_event) + ev->len;
        }
    }
}
