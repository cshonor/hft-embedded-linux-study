/* math_utils.h —— 对外只暴露接口，隐藏实现 */
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

double moving_average(const double *prices, int n);
double clamp(double v, double lo, double hi);

#endif /* MATH_UTILS_H */
