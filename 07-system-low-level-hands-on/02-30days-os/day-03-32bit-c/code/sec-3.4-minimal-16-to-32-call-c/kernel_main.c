/* Teaching example · C entry after asm mode switch (§3.4.3)
 * Book bootpack.c uses HariMain; kernel_main keeps the name generic for tutorials */

void kernel_main(void)
{
    volatile char *vidmem = (volatile char *)0xB8000;
    const char *str = "Hello 32-bit C!";
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        vidmem[i * 2]     = str[i];
        vidmem[i * 2 + 1] = 0x07;
    }

    for (;;) {
    }
}
