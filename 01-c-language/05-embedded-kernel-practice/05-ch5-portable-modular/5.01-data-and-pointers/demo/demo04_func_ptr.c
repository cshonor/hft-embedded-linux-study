#include <stdio.h>

typedef int (*calc_fn)(int, int);

static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }
static int mul(int a, int b) { return a * b; }
static int div_safe(int a, int b) { return b ? a / b : 0; }

typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_COUNT } op_t;

static calc_fn jump_table[OP_COUNT] = {
    [OP_ADD] = add,
    [OP_SUB] = sub,
    [OP_MUL] = mul,
    [OP_DIV] = div_safe,
};

static int dispatch(op_t op, int x, int y)
{
    if (op >= OP_COUNT || !jump_table[op])
        return 0;
    return jump_table[op](x, y);
}

int main(void)
{
    printf("10 + 3 = %d\n", dispatch(OP_ADD, 10, 3));
    printf("10 - 3 = %d\n", dispatch(OP_SUB, 10, 3));
    printf("10 * 3 = %d\n", dispatch(OP_MUL, 10, 3));
    printf("10 / 3 = %d\n", dispatch(OP_DIV, 10, 3));

    calc_fn fp = add;
    printf("fp(7,8)=%d  fn addr=%p\n", fp(7, 8), (void *)fp);
    return 0;
}
