#include <stdio.h>
#include <stdlib.h>

int g_init = 1;
int g_bss;

void show(const char *name, const void *p)
{
    printf("%-12s %p\n", name, p);
}

int main(void)
{
    int local = 2;
    static int s_local = 3;
    void *heap = malloc(16);

    show("text/main", (void *)main);
    show("rodata", (void *)"hello");
    show("data g_init", &g_init);
    show("bss g_bss", &g_bss);
    show("static", &s_local);
    show("stack local", &local);
    show("heap", heap);

    free(heap);
    return 0;
}
