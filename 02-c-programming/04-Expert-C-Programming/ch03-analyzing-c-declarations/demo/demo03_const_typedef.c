#include <stdio.h>

typedef char *Str;

int main(void)
{
    const Str s = "test";
    /* s = NULL; */   /* error: pointer itself is const */
    s[0] = 'T';       /* OK: pointed-to char is mutable */
    printf("%s\n", s);
    return 0;
}
