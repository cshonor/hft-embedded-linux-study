/*
 * c1_2_shrink_demo.c —— 「最小可复现」的实体：一个只在特定记录上炸的程序
 *
 * 用途：1.3 讲「缩小复现」和「数据二分」。这份程序读 N 条行情记录，
 *       对每条记录算一次均价 = 成交额 / 成交量。**其中第 7 条记录的成交量是 0**，
 *       除零直接 SIGFPE —— 于是「缩小到哪一条」就成了可自动判定、可逐步二分的事情。
 *
 * 设计要点（为什么这样写）：
 *   1. 每条记录处理前先打印一行进度 —— 让「崩在第几条」肉眼可见，二分才有的放矢；
 *   2. 输入走 stdin，条数由输入决定 —— 于是「砍输入」是写文件，不是改代码；
 *   3. 除了那一行除零，程序不做任何多余的事 —— 已经是「最小」形态，
 *      对照 1.3 的「砍输入 / 砍线程 / 砍功能 / 砍代码」四步。
 *
 * 用法：
 *   printf '...' > data.txt          # 见 code/README.md 的记录格式
 *   ./c1_2_shrink_demo < data.txt
 *
 * 编译：
 *   gcc -g -O0 -Wall -Wextra -o c1_2_shrink_demo c1_2_shrink_demo.c
 */
#include <stdio.h>
#include <stdlib.h>

struct tick {
    long ts_ms;
    long volume;        /* 成交量（手） */
    long turnover;      /* 成交额（元） */
};

int main(void)
{
    struct tick t;
    long seq = 0;

    while (scanf("%ld %ld %ld", &t.ts_ms, &t.volume, &t.turnover) == 3) {
        seq++;
        printf("record %ld: ts=%ld vol=%ld turnover=%ld\n",
               seq, t.ts_ms, t.volume, t.turnover);
        fflush(stdout);

        /* 均价 = 成交额 / 成交量。vol == 0 的行情（停牌 / 集合竞价前的空档）会除零 */
        long vwap = t.turnover / t.volume;
        printf("            vwap = %ld\n", vwap);
    }

    printf("全部 %ld 条记录处理完毕，未触发问题\n", seq);
    return 0;
}
