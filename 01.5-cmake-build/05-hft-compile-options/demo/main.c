#include <stdio.h>
#include <string.h>
#include <time.h>

/* 行情热路径的微缩模型：ring buffer 解包 + 移动平均。
 * 配 dev preset（ASan+UBSan）跑一遍应当零报告；
 * 故意留一处越界注释，取消注释即可看 ASan 长什么样。 */

#define RING_CAP 1024

typedef struct {
    double  px;
    long    ts_ns;
} tick_t;

static tick_t  ring[RING_CAP];
static size_t  ring_len;

static void ring_push(tick_t t)
{
    /* 越界演示：把 RING_CAP 改成 RING_CAP + 1，dev preset 下 ASan 立刻报
     * heap/stack-buffer-overflow——这就是 5.5 说的"开发期抓现行" */
    if (ring_len < RING_CAP)
        ring[ring_len++] = t;
}

static double ma_tail(size_t n)
{
    double s = 0.0;
    size_t start = ring_len > n ? ring_len - n : 0;
    for (size_t i = start; i < ring_len; i++)
        s += ring[i].px;
    size_t cnt = ring_len - start;
    return cnt ? s / (double)cnt : 0.0;
}

int main(void)
{
    struct timespec ts;
    for (int i = 0; i < 200; i++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ring_push((tick_t){ .px = 100.0 + i * 0.01, .ts_ns = ts.tv_nsec });
    }
    printf("ma_tail(50) = %.4f  (ring=%zu)\n", ma_tail(50), ring_len);
    return 0;
}
