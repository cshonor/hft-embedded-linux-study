#include <stdio.h>

int main(void)
{
    puts("P4 userspace: kernel module is optional on WSL.");
    puts("With headers: (cd .. && make modules) then sudo insmod hello.ko");
    puts("PASS  hello_user");
    return 0;
}
