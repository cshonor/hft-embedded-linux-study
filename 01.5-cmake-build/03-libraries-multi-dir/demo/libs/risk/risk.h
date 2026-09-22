#ifndef RISK_H
#define RISK_H

#include <stddef.h>

/* 对外只暴露"夹取后的均值"——调用方不需要知道 ma 库的存在 */
double risk_clamp_ma(const double *px, size_t n);

#endif
