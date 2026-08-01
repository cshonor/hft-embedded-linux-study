#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void alloc_str(char **out, const char *src)
{
    size_t n = strlen(src) + 1;
    *out = malloc(n);
    if (*out)
        memcpy(*out, src, n);
}

int main(void)
{
    char *s = NULL;
    alloc_str(&s, "double pointer out-param");
    if (s) {
        puts(s);
        free(s);
    }
    return 0;
}
