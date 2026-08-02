/* atexit handlers run in reverse registration order (LIFO).
 * cc -Wall -Wextra -o atexit_order atexit_order.c && ./atexit_order
 */
#include <stdio.h>
#include <stdlib.h>

static void first(void)  { printf("atexit: first (registered 1st, runs last)\n"); }
static void second(void) { printf("atexit: second\n"); }
static void third(void)  { printf("atexit: third (registered last, runs first)\n"); }

int main(void)
{
    atexit(first);
    atexit(second);
    atexit(third);
    printf("main returning → exit path\n");
    return 0;
}
