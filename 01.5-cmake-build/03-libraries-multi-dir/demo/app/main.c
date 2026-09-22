#include <stdio.h>
#include "risk.h"   /* 只 include 风控库的头，ma 对应用层不可见 */

int main(void)
{
    double ticks[] = {100.10, 100.30, 100.20, 100.90, 100.90};
    size_t n = sizeof ticks / sizeof ticks[0];
    printf("n=%zu  reported=%.4f\n", n, risk_clamp_ma(ticks, n));
    return 0;
}
