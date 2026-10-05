#include "log.h"

/* 情形 B/C：内部链接且无人调用。
 * gcc -O0 保留（-> 缺 log.o 时链接报错）；
 * gcc -O2 / clang 任意级别消除（-> 链接通过）。 */
static int multiply(int a, int b)
{
    logmsg("mul");
    return a * b;
}
