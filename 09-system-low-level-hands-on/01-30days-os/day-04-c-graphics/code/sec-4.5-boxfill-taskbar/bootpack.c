/* Day 4 §4.5 · boxfill 矩形 + 底部任务条雏形 */

void io_hlt(void);

#define VRAM_BASE  0xA0000
#define SCREEN_W   320
#define SCREEN_H   200

static void putpixel(volatile unsigned char *vram, int x, int y, unsigned char c)
{
    vram[y * SCREEN_W + x] = c;
}

static void boxfill(volatile unsigned char *vram,
                    unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            putpixel(vram, x, y, c);
        }
    }
}

void HariMain(void)
{
    volatile unsigned char *vram = (volatile unsigned char *)VRAM_BASE;

    boxfill(vram, 15, 0, 0, SCREEN_W - 1, SCREEN_H - 1);   /* 背景白 */
    boxfill(vram, 0, 0, 180, SCREEN_W - 1, SCREEN_H - 1);    /* 底 20px 黑条 ≈ 任务条 */

    for (;;) {
        io_hlt();
    }
}
