#include <stdio.h>
#include <string.h>

int main(void)
{
    char dst[4];
    const char *src = "hello";

    strncpy(dst, src, sizeof(dst));
    printf("strncpy(4) without manual NUL: ");
    for (int i = 0; i < 8; i++)
        printf("%02x ", (unsigned char)dst[i]);
    printf("\n");

    strncpy(dst, src, sizeof(dst) - 1);
    dst[sizeof(dst) - 1] = '\0';
    printf("safe copy: [%s]\n", dst);

    printf("strlen(\"abc\")=%zu, need buf[%zu]\n",
           strlen("abc"), strlen("abc") + 1);

    return 0;
}
