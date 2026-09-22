/* math_utils.c —— 示例工程的实现文件 */
#include "math_utils.h"

/* 简单移动平均：HFT 里最平凡却最常见的一类计算 */
double moving_average(const double *prices, int n)
{
    if (n <= 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += prices[i];
    }
    return sum / n;
}

/* 把数值夹进 [lo, hi] */
double clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
