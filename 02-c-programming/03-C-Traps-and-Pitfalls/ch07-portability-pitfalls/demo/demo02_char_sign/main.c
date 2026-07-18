#include <limits.h>
#include <stdio.h>

int main(void)
{
    char c = (char)0x80;
    signed char sc = (signed char)0x80;
    unsigned char uc = 0x80;

    printf("CHAR_MIN=%d CHAR_MAX=%d\n", CHAR_MIN, CHAR_MAX);
    printf("plain char 0x80: c<0 ? %s\n", (c < 0) ? "yes" : "no");
    printf("signed char 0x80: sc<0 ? %s\n", (sc < 0) ? "yes" : "no");
    printf("unsigned char 0x80: uc<0 ? %s\n", (uc < 0) ? "yes" : "no");

    printf("binary bytes: use unsigned char / uint8_t\n");

    return 0;
}
