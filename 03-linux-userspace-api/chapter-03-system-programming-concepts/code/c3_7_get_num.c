/* TLPI 第 03 章 §3.5.2 —— 原书 get_num.c 要解决的问题：把「字符串读数字」
 * 里「0」和「出错」这两件完全不同的事分开。
 *
 * atoi(3) 的致命缺陷（man 原文）："The atoi() function ... has undefined behavior
 * if the value cannot be represented" —— 而且它**完全没有错误返回通道**：
 *   atoi("abc") == 0   也  atoi("0") == 0
 * 你还可能是 ERANGE 溢出，它照样给你一个截断后的值。
 *
 * 原书 lib/get_num.c（Listing 3-6）的做法 = strtol + errno + endptr 三态判定。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_7 c3_7_get_num.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- 反面教材：atoi 说不清到底发生了什么 ---------- */
static void show_atoi(const char *s)
{
    errno = 0;
    int v = atoi(s);
    printf("  atoi(%-28s) = %-12d errno = %d\n", s, v, errno);
}

/* ---------- 正面写法：原书 get_num.c 的三态判定 ---------- */
enum parse_status { OK, BAD_TOKEN, OVERFLOW, EMPTY };

static enum parse_status get_int(const char *str, int *out)
{
    if (str == NULL || *str == '\0') {
        return EMPTY;
    }
    errno = 0;
    char *endptr = NULL;
    long v = strtol(str, &endptr, 10);

    if (errno != 0) {              /* ① 溢出：strtol 设 ERANGE */
        return OVERFLOW;
    }
    if (endptr == str || *endptr != '\0') {   /* ② 一个数字字符都没吃 / 后面还有垃圾 */
        return BAD_TOKEN;
    }
    if (v < INT_MIN || v > INT_MAX) {         /* ③ long 装得下但 int 装不下 */
        return OVERFLOW;
    }
    *out = (int) v;
    return OK;
}

static const char *status_name(enum parse_status s)
{
    switch (s) {
    case OK:        return "OK";
    case BAD_TOKEN: return "BAD_TOKEN";
    case OVERFLOW:  return "OVERFLOW";
    case EMPTY:     return "EMPTY";
    }
    return "?";
}

int main(void)
{
    printf("=== ① atoi 的问题：0 与出错无法区分 ===\n");
    show_atoi("0");
    show_atoi("abc");
    show_atoi("");
    show_atoi("123abc");                       /* 只吃前缀，后半段被静默丢掉 */
    show_atoi("99999999999999999999");         /* 远超 int 范围 */
    show_atoi("-99999999999999999999");
    printf("  → 前两行的返回值一模一样，都是 0；最后两行的溢出也不报错\n\n");

    printf("=== ② 同一个输入用 strtol + errno + endptr 判 ===\n");
    const char *cases[] = {"0", "abc", "", "123abc", "99999999999999999999",
                           "-99999999999999999999", " 42", "42 ", "+7", "0x1f", "+0", NULL};
    for (int i = 0; cases[i] != NULL; i++) {
        int v = 0;
        enum parse_status st = get_int(cases[i], &v);
        printf("  get_int(%-24s) → %-10s", cases[i], status_name(st));
        if (st == OK) {
            printf(" out = %d", v);
        }
        printf("\n");
    }
    printf("\n");

    printf("=== ③ 朴素解法绕不过去的角落 ===\n");
    printf("  \" 42\"  → strtol 跳过前导空白后成功（返回 42）—— 与 atoi 行为一致\n");
    printf("  \"42 \"  → strtol 会在空格处停，endptr 指向空格 → 判 BAD_TOKEN\n");
    printf("            想接受尾随空白，得自己跳过 endptr 处的 isspace()\n");
    printf("  \"0x1f\" → 基数为 10 时只吃 \"0\"，endptr 指向 'x' → BAD_TOKEN\n");
    printf("            真正的十六进制要用基数 0（自动判别前缀）或 16\n");
    printf("  \"+7\"、\"-0\"、\"007\" 都是合法的，strtol 照收\n\n");

    printf("=== ④ 原书 get_num.c 的返回值语义（复刻其判据）===\n");
    printf("  它返回 int 而不是状态枚举，用「返回 -1 表示出错」的风格，\n");
    printf("  并在函数里直接 errExit(\"strtol\")，所以调用者只需处理「拿到数字」这一种情况。\n");
    printf("  两种风格都行，关键是：**必须有三态**（成功 / 非法 / 溢出），不能只有两态\n");
    return 0;
}
