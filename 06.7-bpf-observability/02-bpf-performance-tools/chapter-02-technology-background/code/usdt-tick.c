/* usdt-tick.c — USDT 静态探针最小 demo（02-ch02 §10 块1/3/4 补全）
 *
 * USDT = 应用内嵌的静态探针：内核不需要改，bpftrace 按 provider:name 挂。
 * 与 uprobe 的区别：uprobe 挂"函数边界"，USDT 挂"逻辑位置"（函数中间任意点）。
 *
 * 编译: gcc -g -O2 -o usdt-tick usdt-tick.c
 *   依赖: sys/sdt.h（sudo apt install systemtap-sdt-dev）
 * 验证探针已写进二进制（块2）:
 *   readelf -n usdt-tick | grep -A3 stapsdt
 * 挂载（块4，需 root）:
 *   sudo bpftrace -e 'usdt:./usdt-tick:loop { printf("got: %d\n", arg0); }'
 *   另开终端跑 ./usdt-tick → got: 1 / got: 2 / got: 3 ...
 */
#include <sys/sdt.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    for (int i = 1; ; i++) {
        /* 探针：provider=tick, name=loop，携带一个 int 参数
         * 未激活时 ≈ 一条 nop，开销可忽略（这就是 USDT 的卖点） */
        DTRACE_PROBE1(tick, loop, i);

        /* semaphore 版本（块3）：昂贵参数只在探针激活时才算
         * if (DTRACE_PROBE1_ENABLED(tick, expensive)) {
         *     DTRACE_PROBE1(tick, expensive, compute_costly_arg());
         * } */
        printf("tick %d\n", i);
        sleep(1);
    }
    return 0;
}
