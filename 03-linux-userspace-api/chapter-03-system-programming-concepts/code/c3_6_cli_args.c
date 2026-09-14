/* TLPI 第 03 章 §3.5.1 —— 命令行选项与参数：argc/argv 的形状 + getopt(3)
 *
 * 原书在 §3.5.1 说明「本书示例程序统一用什么方式解析命令行」，用到的就是
 * getopt(3)。本 demo 把解析逻辑做成函数，然后**喂人工构造的 argv** ——
 * 这样实验可控（CE 上真实 argv 只有 ./output.s），也顺带演示了
 * 「argc/argv 只是一个数组，没有魔法」。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_6 c3_6_cli_args.c
 */
#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- ① argv 的形状：最后一个元素必然是 NULL ---------- */

static void show_argv(int argc, char **argv, const char *tag)
{
    printf("  [%s] argc = %d\n", tag, argc);
    for (int i = 0; i < argc; i++) {
        printf("      argv[%d] = \"%s\"\n", i, argv[i]);
    }
    printf("      argv[%d] = %s   ← 标准保证最后一个元素是 NULL\n", argc,
           argv[argc] == NULL ? "NULL" : "(非 NULL，不合法！)");
}

/* ---------- ② getopt(3)：`-a` / `-b val` / `-c` / 非法选项 ---------- */

static void parse(int argc, char **argv, const char *tag)
{
    printf("  [%s] 解析 \"%s\"（getopt 选项串 \"ab:c\"）\n", tag, argc > 1 ? argv[1] : "");
    optind = 1;          /* getopt 的状态是全局的，重入前必须复位 */
    opterr = 0;          /* 关掉 getopt 自己打的错误信息，改由我们统一报 */
    optarg = NULL;

    int ch;
    while ((ch = getopt(argc, argv, "ab:c")) != -1) {
        switch (ch) {
        case 'a':
            printf("      -a 开关，无参数\n");
            break;
        case 'b':
            printf("      -b 带参数，optarg = \"%s\"\n", optarg);
            break;
        case 'c':
            printf("      -c 开关，无参数\n");
            break;
        case '?':
            if (optopt) {
                printf("      非法选项 '-%c'（未知选项）→ 本该 usageErr 并退出\n", optopt);
            } else {
                printf("      '-%s' 缺少必需参数 → 本该 usageErr 并退出\n",
                       argv[optind - 1] ? argv[optind - 1] : "?");
            }
            break;
        default:
            printf("      getopt 返回了意外字符 '%c'\n", ch);
            break;
        }
    }
    printf("      解析结束 optind = %d\n", optind);
    for (int i = optind; i < argc; i++) {
        printf("      非选项参数（操作数）: \"%s\"\n", argv[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== ① 本次真正的 argv（CE 上只给了程序名）===\n");
    show_argv(argc, argv, "真实 argv");
    printf("\n");

    printf("=== ② 人工构造的 argv：演示 getopt 的三种结局 ===\n");
    char *t1[] = {"prog", "-a", "-b", "42", "-c", "file1", "file2", NULL};
    char *t2[] = {"prog", "-x", NULL};
    char *t3[] = {"prog", "-b", NULL};
    char *t4[] = {"prog", "standalone.txt", NULL};
/* 用 sizeof 算 argc，别手数 —— 手数一次就会错 */
#define ARGC(a) ((int) (sizeof(a) / sizeof((a)[0]) - 1))
    parse(ARGC(t1), t1, "全部正常");
    parse(ARGC(t2), t2, "未知选项 -x");
    parse(ARGC(t3), t3, "缺参数 -b");
    parse(ARGC(t4), t4, "只有操作数");

    printf("=== ③ 为什么 GLIBC 的 getopt 支持 \"--long\"：getopt_long ===\n");
    printf("  POSIX 的 getopt 只认单字符选项；GNU 扩展 getopt_long/getopt_long_only\n");
    printf("  才认 \"--verbose\" 这种长选项（需要 <getopt.h>）\n\n");

    printf("=== ④ 原书示例程序的统一约定（§3.5.1）===\n");
    printf("  - 用法：./prog [-a] [-b value] file...\n");
    printf("  - 参数错 → usageErr(\"%%s [-a] [-b value] file...\", prog) 打到 stderr 并退出\n");
    printf("  - usageErr 的实现见 c3_8_error_functions.c\n");
    return 0;
}
