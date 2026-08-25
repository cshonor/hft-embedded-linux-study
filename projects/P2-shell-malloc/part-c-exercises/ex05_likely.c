#include <stdio.h>

/* GNU C：告诉编译器哪个分支更常走，方便排指令。不是「保证预测成功」。 */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

int main(void)
{
    int n = 0;
    for (int i = 0; i < 1000; i++) {
        if (likely(i >= 0))
            n++;
        if (unlikely(i == -1))
            return 1;
    }
    if (n != 1000)
        return 1;
    puts("likely/unlikely compiled");
    return 0;
}
