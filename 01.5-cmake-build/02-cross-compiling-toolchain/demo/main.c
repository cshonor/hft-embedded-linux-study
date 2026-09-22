#include <stdio.h>
#include "math_utils.h"

/* 与第 1 章同款 demo：5 笔成交价的移动平均，夹到风控区间 */
int main(void)
{
    double ticks[] = {100.10, 100.30, 100.20, 100.90, 100.90};
    size_t n = sizeof ticks / sizeof ticks[0];
    double ma = moving_avg(ticks, n);
    printf("n=%zu  MA=%.4f  reported=%.4f\n", n, ma, risk_clamp(ma));
    return 0;
}
