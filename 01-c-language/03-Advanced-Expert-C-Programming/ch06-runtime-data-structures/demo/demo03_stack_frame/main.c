#include <stdio.h>

static void C(void)
{
    int c_local = 3;
    printf("  C frame: &c_local=%p\n", (void *)&c_local);
}

static void B(void)
{
    int b_local = 2;
    printf(" B frame: &b_local=%p\n", (void *)&b_local);
    C();
}

static void A(void)
{
    int a_local = 1;
    printf("A frame: &a_local=%p\n", (void *)&a_local);
    B();
}

int main(void)
{
    int main_local = 0;
    printf("main frame: &main_local=%p\n", (void *)&main_local);
    A();
    return 0;
}
