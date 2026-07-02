/* Day 4 §4.1 · VRAM white fill + stripes (haribote / QEMU) */

void io_hlt(void);

#define VRAM_BASE   0xA0000
#define SCREEN_W    320
#define SCREEN_H    200
#define TOTAL_PIX   (SCREEN_W * SCREEN_H)
#define PIXEL_BLACK 0
#define PIXEL_WHITE 15

static void fill_screen(volatile unsigned char *vram, unsigned char color)
{
    int i;

    for (i = 0; i < TOTAL_PIX; i++) {
        vram[i] = color;
    }
}

static void draw_stripes_x_mask(volatile unsigned char *vram)
{
    int x, y;

    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            vram[y * SCREEN_W + x] = (x & 8) ? PIXEL_BLACK : PIXEL_WHITE;
        }
    }
}

static void draw_stripes_xor_cols(volatile unsigned char *vram)
{
    int i;

    for (i = 0; i < TOTAL_PIX; i++) {
        vram[i] = (i & 1) ? PIXEL_WHITE : PIXEL_BLACK;
    }
}

void HariMain(void)
{
    volatile unsigned char *vram = (volatile unsigned char *)VRAM_BASE;

    fill_screen(vram, PIXEL_WHITE);
    /* draw_stripes_x_mask(vram); */
    /* draw_stripes_xor_cols(vram); */

    for (;;) {
        io_hlt();
    }
}
