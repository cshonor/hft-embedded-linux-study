#include <ctype.h>
#include <stdio.h>

int main(void)
{
    signed char sch = (signed char)0xFF;
    unsigned char uch = 0xFF;

    printf("sch as int=%d (negative on typical platform)\n", (int)sch);
    printf("uch as int=%u\n", (unsigned)uch);

    printf("WRONG isprint(sch): %d\n", isprint(sch));
    printf("SAFE  isprint((unsigned char)sch): %d\n",
           isprint((unsigned char)sch));

    return 0;
}
