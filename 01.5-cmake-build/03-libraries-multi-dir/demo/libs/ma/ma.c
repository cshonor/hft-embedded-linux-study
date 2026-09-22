#include "ma.h"

double ma_compute(const double *px, size_t n)
{
    double s = 0.0;
    for (size_t i = 0; i < n; i++)
        s += px[i];
    return n ? s / (double)n : 0.0;
}
