/* c1_5_pipeline.c —— 1.5 全链路笔记的实测标本
 *
 * 用途：把「源码 → gcc -g → DWARF → gdb/strace」这条链路上的每一步
 *       都在这个 30 行的小程序上实测：段数量、文件体积、行号表条目。
 *
 * 编译：clang -g -O0 -o c1_5_pipeline c1_5_pipeline.c
 * 跑：  ./c1_5_pipeline 10 99
 */
#include <stdio.h>
#include <stdlib.h>

static int apply_fee(int qty, int price) {
    int notional = qty * price;          /* 第 13 行：行号表要指向的"案发现场" */
    return notional;
}

int main(int argc, char **argv) {
    int qty   = argc > 1 ? atoi(argv[1]) : 10;
    int price = argc > 2 ? atoi(argv[2]) : 99;
    int total = apply_fee(qty, price);   /* 第 20 行：gdb break 就打在这里 */
    printf("total=%d\n", total);
    return 0;
}
