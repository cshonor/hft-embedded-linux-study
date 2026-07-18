#include <stdio.h>
#include <string.h>

int main(void)
{
    char str_buf[2][8] = {"hello", "c"};
    char *str_ptr[2] = {"hello", "c"};

    printf("=== char s[N][M] 二维字符数组 ===\n");
    printf("sizeof str_buf=%zu\n", sizeof str_buf);
    str_buf[0][0] = 'H';
    printf("可写: %s\n", str_buf[0]);

    printf("\n=== char *s[N] 指针数组 ===\n");
    printf("sizeof str_ptr=%zu (仅指针)\n", sizeof str_ptr);
    printf("rodata: %s %s\n", str_ptr[0], str_ptr[1]);
    /* str_ptr[0][0] = 'H';  // 取消注释 → SIGSEGV */

    (void)str_ptr;
    return 0;
}
