#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int append_string(char ***list, size_t *count, const char *s)
{
    char *copy = strdup(s);
    if (!copy)
        return -1;
    char **new_list = realloc(*list, (*count + 1) * sizeof(char *));
    if (!new_list) {
        free(copy);
        return -1;
    }
    *list = new_list;
    (*list)[*count] = copy;
    (*count)++;
    return 0;
}

int main(void)
{
    char **argv_like = NULL;
    size_t n = 0;

    append_string(&argv_like, &n, "uart0");
    append_string(&argv_like, &n, "spi1");
    append_string(&argv_like, &n, "i2c2");

    printf("string list via char ** (count=%zu):\n", n);
    for (size_t i = 0; i < n; i++)
        printf("  [%zu] %s @ %p\n", i, argv_like[i], (void *)argv_like[i]);

    for (size_t i = 0; i < n; i++)
        free(argv_like[i]);
    free(argv_like);
    return 0;
}
