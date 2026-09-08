/* T12 C side: the C function called by t12_asm_calls_c.S
 * plus a main to drive it. */
#include <stdio.h>

int c_add(int a, int b)
{
    return a + b;
}

extern int asm_call_c(int a, int b);

int main(void)
{
    /* asm_call_c(3,5) should call c_add(5,3) and return 8 */
    int r = asm_call_c(3, 5);
    printf("asm_call_c(3,5) = %d (should be 8)\n", r);
    return 0;
}
