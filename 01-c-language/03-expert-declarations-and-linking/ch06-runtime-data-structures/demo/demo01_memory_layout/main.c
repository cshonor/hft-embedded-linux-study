#include <stdio.h>
#include <stdlib.h>

int global_init = 1;
int global_uninit;
static int file_static; /* .bss */

static const char *ro_msg = "hello .rodata";

static void test(void)
{
    int stack_var = 10;
    static int static_init = 20;
    static int static_uninit;
    char *heap_ptr = malloc(16);

    printf("=== 运行时地址（Linux 典型布局）===\n");
    printf(".text       test: %p\n", (void *)test);
    printf(".rodata     ro_msg: %p\n", (void *)ro_msg);
    printf(".data       global_init: %p\n", (void *)&global_init);
    printf(".data       static_init: %p\n", (void *)&static_init);
    printf(".bss        global_uninit: %p\n", (void *)&global_uninit);
    printf(".bss        static_uninit: %p\n", (void *)&static_uninit);
    printf(".bss        file_static: %p\n", (void *)&file_static);
    printf("heap        malloc: %p\n", (void *)heap_ptr);
    printf("stack       stack_var: %p\n", (void *)&stack_var);

    free(heap_ptr);
}

int main(void)
{
    test();
    return 0;
}
