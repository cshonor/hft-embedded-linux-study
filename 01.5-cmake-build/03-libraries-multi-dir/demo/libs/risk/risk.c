#include "risk.h"
#include "ma.h"   /* 内部实现用 ma，但 risk.h 不暴露它 */

double risk_clamp_ma(const double *px, size_t n)
{
    double ma = ma_compute(px, n);
    if (ma > 105.0) return 105.0;
    if (ma < 95.0)  return 95.0;
    return ma;
}
