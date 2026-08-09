#include <stdio.h>
#include <string.h>

enum kind { K_INT, K_STR };

struct value {
    enum kind k;
    union {
        int i;
        char s[32];
    } u;
};

static void print_value(const struct value *v)
{
    if (v->k == K_INT)
        printf("int=%d\n", v->u.i);
    else
        printf("str=%s\n", v->u.s);
}

int main(void)
{
    struct value a = {.k = K_INT, .u.i = 7};
    struct value b = {.k = K_STR};
    strncpy(b.u.s, "pi", sizeof b.u.s - 1);
    print_value(&a);
    print_value(&b);
    return 0;
}
