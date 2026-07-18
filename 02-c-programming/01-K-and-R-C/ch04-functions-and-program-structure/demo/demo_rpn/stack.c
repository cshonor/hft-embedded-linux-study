#include <stdio.h>
#include "calc.h"

static int stack_buf[MAX_STACK];
int stack_ptr = 0;

void push(int val)
{
    if (stack_ptr >= MAX_STACK) {
        printf("栈溢出\n");
        return;
    }
    stack_buf[stack_ptr++] = val;
}

int pop(void)
{
    if (stack_ptr <= 0) {
        printf("栈为空\n");
        return 0;
    }
    return stack_buf[--stack_ptr];
}
