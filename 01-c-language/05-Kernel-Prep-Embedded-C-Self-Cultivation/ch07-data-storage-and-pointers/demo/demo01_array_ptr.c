#include <stdio.h>

static void show_size(const char *name, size_t sz, const void *addr)
{
    printf("%-20s sizeof=%zu  addr=%p\n", name, sz, addr);
}

static void pass_array(char arr[16])
{
    printf("  pass_array: sizeof(arr)=%zu (decayed to pointer)\n", sizeof arr);
}

int main(void)
{
    char buf[16] = "hello";
    char *p = buf;

    show_size("buf (array)", sizeof buf, buf);
    show_size("p (pointer)", sizeof p, &p);
    show_size("*p (first byte)", sizeof *p, p);

    printf("buf=%p  p=%p  &buf=%p\n", (void *)buf, (void *)p, (void *)&buf);
    /* buf = p;  // error: array name is not assignable */

    p = buf + 1;
    printf("after p=buf+1: *p='%c'\n", *p);

    pass_array(buf);
    return 0;
}
