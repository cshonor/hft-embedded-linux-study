#include <stdio.h>

void test_switch(void)
{
    int x = 2;
    switch (x) {
    case 1: puts("case1"); break;
    case 2: puts("case2"); /* fall through */
    case 3: puts("case3");
    }
}

int main(void)
{
    test_switch();
    puts("expected: case2 then case3");
    return 0;
}
