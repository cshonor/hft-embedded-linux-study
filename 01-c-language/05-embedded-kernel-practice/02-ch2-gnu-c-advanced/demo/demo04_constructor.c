#include <stdio.h>

static void __attribute__((constructor(101))) early_init(void)
{
    printf("[constructor] runs before main (priority 101)\n");
}

static void __attribute__((constructor)) normal_init(void)
{
    printf("[constructor] runs before main (default priority)\n");
}

static void __attribute__((destructor)) cleanup(void)
{
    printf("[destructor] runs after main returns\n");
}

int main(void)
{
    puts("[main] application entry");
    return 0;
}
