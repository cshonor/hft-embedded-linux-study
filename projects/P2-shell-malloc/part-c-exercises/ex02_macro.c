#include <stdio.h>

#define STR(x) #x
#define DBG(expr) printf("%s:%d %s = %d\n", __FILE__, __LINE__, STR(expr), (expr))

int main(void)
{
    int x = 42;
    DBG(x);
    DBG(x + 1);
    return 0;
}
