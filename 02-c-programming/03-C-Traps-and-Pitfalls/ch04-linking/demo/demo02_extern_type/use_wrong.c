#include <stdio.h>

extern char *msg;

int main(void)
{
    printf("wrong extern char *msg: [%s]\n", msg);
    return 0;
}
