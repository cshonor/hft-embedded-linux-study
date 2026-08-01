/* Day 3 · bootpack.c — OS main logic in 32-bit protected mode
 * Entry: HariMain (called from nasmhead.asm after mode switch)
 */

void io_hlt(void);

void HariMain(void)
{
    volatile unsigned char *vram = (volatile unsigned char *)0xA0000;
    int i;

    /* §3.2 acceptance: fill mode-0x13 framebuffer with palette 0 (black) */
    for (i = 0; i < 320 * 200; i++) {
        vram[i] = 0;
    }

    for (;;) {
        io_hlt();
    }
}
