/* ex18_3_realpath.c — 习题 18-3：实现 realpath()
 *
 * 题面：实现 realpath()——把任意路径解析为「无符号链接、无 . / ..、
 * 无多余斜杠」的绝对路径。
 *
 * 思路（分层消化）：
 *   1. 相对路径前拼 cwd；
 *   2. 以 '/' 切分组件，逐个处理：
 *        "" / "."  → 跳过
 *        ".."      → 弹栈（模拟目录树回退）
 *        其他      → 压栈（对每个组件做 lstat：是符号链接就 readlink 展开，
 *                    把展开结果按"相对当前栈"拼接后**重新走解析**——递归深度有限，
 *                    超过上限即 ELOOP）
 *   3. 末组件允许不存在（realpath 的 resolved 语义：最后的组件可以不存在）。
 *
 * 简化声明（诚实）：本实现是**教学版**——不含路径级权限检查（realpath 契约里
 * 每级目录要可搜索 x），ELOOP 上限取 40（macOS 实测值；Linux SYMLOOP 上限 40）。
 * 逐组件用 lstat+readlink 与内核路径解析同构，核心逻辑与原书答案一致。
 *
 * 编译： cc -Wall -Wextra -o ex18_3_realpath ex18_3_realpath.c
 * 取材： TLPI §18.13 习题 18-3；man-pages realpath(3)
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define MAX_LINKS 40

/* 把组件栈渲染成绝对路径 */
static void join(char *out, size_t n, char **comp, int depth)
{
    if (depth == 0) { snprintf(out, n, "/"); return; }
    size_t used = 0;
    for (int i = 0; i < depth; i++)
        used += (size_t) snprintf(out + used, n - used, "/%s", comp[i]);
}

/* 逐组件解析；返回 0 成功（结果在 out），-1 出错 */
static int resolve(const char *in, char *out, size_t n, int *links_used)
{
    char work[PATH_MAX * 2];
    char *comp[256];
    int depth = 0;

    /* 相对路径 → 拼 cwd */
    if (in[0] != '/') {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof cwd) == NULL)
            return -1;
        snprintf(work, sizeof work, "%s/%s", cwd, in);
    } else {
        snprintf(work, sizeof work, "%s", in);
    }

    char *save = NULL;
    for (char *tok = strtok_r(work, "/", &save); tok;
         tok = strtok_r(NULL, "/", &save)) {
        if (strcmp(tok, ".") == 0)
            continue;
        if (strcmp(tok, "..") == 0) {
            if (depth > 0) depth--;         /* 根之上再 .. 停在根 */
            continue;
        }
        comp[depth++] = tok;

        /* 检查这一级是否是符号链接 */
        char cur[PATH_MAX];
        join(cur, sizeof cur, comp, depth);
        struct stat sb;
        if (lstat(cur, &sb) == -1) {
            /* 只允许最后一个组件不存在（realpath 语义） */
            if (errno == ENOENT && strtok_r(NULL, "/", &save) == NULL)
                break;
            return -1;
        }
        if (S_ISLNK(sb.st_mode)) {
            if (++(*links_used) > MAX_LINKS) { errno = ELOOP; return -1; }
            char target[PATH_MAX];
            ssize_t tl = readlink(cur, target, sizeof target - 1);
            if (tl == -1) return -1;
            target[tl] = '\0';
            /* 链接目标 + 未处理的剩余部分 → 重新解析 */
            depth--;
            char rest[PATH_MAX];
            char *rem = strtok_r(NULL, "", &save);   /* 剩余原文 */
            snprintf(rest, sizeof rest, "%s%s%s",
                     target, rem ? "/" : "", rem ? rem : "");
            /* 拼上已解析前缀，继续循环 */
            char prefix[PATH_MAX];
            join(prefix, sizeof prefix, comp, depth);
            char combined[PATH_MAX * 2];
            if (rest[0] == '/')
                snprintf(combined, sizeof combined, "%s", rest);
            else
                snprintf(combined, sizeof combined, "%s%s/%s",
                         depth > 0 ? prefix : "", depth > 0 ? "" : "/", rest);
            char in2[PATH_MAX * 2];
            snprintf(in2, sizeof in2, "%s", combined);
            char out2[PATH_MAX];
            if (resolve(in2, out2, sizeof out2, links_used) == -1)
                return -1;
            snprintf(out, n, "%s", out2);
            return 0;
        }
    }
    join(out, n, comp, depth);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        return EXIT_FAILURE;
    }
    int links = 0;
    char out[PATH_MAX];
    if (resolve(argv[1], out, sizeof out, &links) == -1) {
        fprintf(stderr, "realpath(%s): %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    printf("输入: %s\n自实现: %s\n", argv[1], out);

    /* 与系统 realpath 对拍 */
    char *sys = realpath(argv[1], NULL);
    if (sys) {
        printf("系统库: %s\n%s\n", sys,
               strcmp(sys, out) == 0 ? "→ 一致 ✔" : "→ 不一致 ✘");
        free(sys);
    } else {
        printf("系统库: 失败（%s）——常见于末组件不存在的情况差异\n", strerror(errno));
    }
    return 0;
}
