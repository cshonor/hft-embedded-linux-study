#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* 不是 DPDK：在用户态模拟「两个口之间 busy-poll 转发」，体会 PMD 循环长什么样。 */

#define N 1024

int main(void)
{
    uint32_t rx[N];
    uint32_t tx[N];
    for (int i = 0; i < N; i++)
        rx[i] = (uint32_t)i;

    int copied = 0;
    for (;;) {
        /* 生产里这里是 rte_eth_rx_burst / tx_burst，永不 sleep。 */
        if (copied >= N)
            break;
        tx[copied] = rx[copied];
        copied++;
    }
    if (memcmp(rx, tx, sizeof rx) != 0) {
        fprintf(stderr, "forward mismatch\n");
        return 1;
    }
    printf("part-a-host-poll: forwarded %d pkts (userspace mock, not DPDK)\n", copied);
    return 0;
}
