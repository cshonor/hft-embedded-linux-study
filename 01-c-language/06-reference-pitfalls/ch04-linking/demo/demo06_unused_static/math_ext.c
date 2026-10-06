#include "log.h"

/* 情形 A：外部链接。全程序无人调用，也必须发射进 .o，
 * 体内对 logmsg 的引用一定进未定义符号表。 */
int multiply(int a, int b)
{
    logmsg("mul");
    return a * b;
}
