#include <stdio.h>

static int handler(void)
{
    puts("handler() was CALLED (wrong for registration)");
    return 0;
}

static void register_cb(int (*cb)(void))
{
    if (cb)
        puts("register: got function pointer OK");
    else
        puts("register: got NULL");
}

int main(void)
{
    register_cb(handler);
    puts("---");
    register_cb(handler());
    return 0;
}
