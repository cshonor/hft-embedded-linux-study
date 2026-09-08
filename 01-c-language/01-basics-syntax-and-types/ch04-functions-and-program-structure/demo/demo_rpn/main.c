#include <stdio.h>
#include <stdlib.h>
#include "calc.h"

int main(void)
{
    char token[MAX_TOKEN];
    int op1, op2, type;

    while ((type = getop(token)) != EOF) {
        if (type == '0') {
            push(atoi(token));
        } else if (type == '+' || type == '-' || type == '*' || type == '/') {
            op2 = pop();
            op1 = pop();
            switch (type) {
            case '+': push(op1 + op2); break;
            case '-': push(op1 - op2); break;
            case '*': push(op1 * op2); break;
            case '/': push(op1 / op2); break;
            }
        } else if (type == '\n') {
            printf("结果：%d\n", pop());
        } else {
            printf("非法字符\n");
        }
    }
    return 0;
}
