/* main.c —— 示例工程的主程序 */
#include <stdio.h>
#include "math_utils.h"

int main(void)
{
    double prices[] = { 100.5, 101.0, 99.8, 100.2, 100.9 };
    int n = (int)(sizeof(prices) / sizeof(prices[0]));

    double ma = moving_average(prices, n);
    /* MA 按风控要求夹在 [99, 101] 区间内再上报 */
    double reported = clamp(ma, 99.0, 101.0);

    printf("n=%d  MA=%.4f  reported=%.4f\n", n, ma, reported);
    return 0;
}
