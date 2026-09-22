#include "math_utils.h"

double moving_avg(const double *px, size_t n)
{
    double s = 0.0;
    for (size_t i = 0; i < n; i++)
        s += px[i];
    return n ? s / (double)n : 0.0;
}

double risk_clamp(double px)
{
    if (px > 105.0) return 105.0;
    if (px < 95.0)  return 95.0;
    return px;
}
