/* Day 4 §4.4 · CLI/STI + PUSHFD/POPFD — 设调色板临界区 */

void io_hlt(void);
void io_cli(void);
void io_sti(void);
void set_palette_rgb(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
void palette_init_with_cli(void);

#define VRAM_BASE  0xA0000
#define SCREEN_W   320
#define SCREEN_H   200

static void init_palette_safe(void)
{
    /* 原书风格：关中断期间写完调色板 */
    io_cli();
    set_palette_rgb(0, 0, 0, 0);
    palette_init_with_cli();            /* asm: pushfd + cli + OUT + popfd */
    set_palette_rgb(15, 63, 63, 63);
    io_sti();
}

void HariMain(void)
{
    volatile unsigned char *vram = (volatile unsigned char *)VRAM_BASE;
    int i;

    init_palette_safe();

    for (i = 0; i < SCREEN_W * SCREEN_H; i++)
        vram[i] = 6;                    /* 色号 6 = 上面设的绿 */

    for (;;) {
        io_hlt();
    }
}
