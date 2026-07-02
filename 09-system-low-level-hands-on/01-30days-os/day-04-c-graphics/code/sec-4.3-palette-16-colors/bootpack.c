/* Day 4 §4.3 · 调色板 — 色号索引经 OUT 0x3c8/0x3c9 映射到 RGB */

void io_hlt(void);
void set_palette_rgb(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

#define VRAM_BASE  0xA0000
#define SCREEN_W   320
#define SCREEN_H   200

static void init_palette_demo(void)
{
    /* 改几条调色板项：0=黑，7=红，15=白（教学用，非完整 16 色表） */
    set_palette_rgb(0, 0, 0, 0);
    set_palette_rgb(7, 63, 0, 0);
    set_palette_rgb(15, 63, 63, 63);
}

void HariMain(void)
{
    volatile unsigned char *vram = (volatile unsigned char *)VRAM_BASE;
    int x, y;

    init_palette_demo();

    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            if (x < 100)
                vram[y * SCREEN_W + x] = 0;
            else if (x < 200)
                vram[y * SCREEN_W + x] = 7;
            else
                vram[y * SCREEN_W + x] = 15;
        }
    }

    for (;;) {
        io_hlt();
    }
}
