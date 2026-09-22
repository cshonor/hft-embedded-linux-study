#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stddef.h>

double moving_avg(const double *px, size_t n);
double risk_clamp(double px);

#endif
