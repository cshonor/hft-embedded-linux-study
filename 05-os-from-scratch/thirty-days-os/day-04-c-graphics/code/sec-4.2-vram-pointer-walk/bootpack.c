/* Day 4 §4.2 · char * 写 VRAM — *p、p++、与汇编 [] 寻址对照 */

void io_hlt(void);

#define VRAM_BASE  0xA0000
#define SCREEN_W   320
#define SCREEN_H   200

void HariMain(void)
{
    char *p = (char *)VRAM_BASE;   /* 显存起点 = 一个「地址」 */
    int x, y;

    for (y = 0; y < SCREEN_H; y++) {
        for (x = 0; x < SCREEN_W; x++) {
            /* 左半 *p=0 + p++；右半 *p=15 + p++ — 不用 vram[y*320+x] */
            if (x < SCREEN_W / 2)
                *p = 0;
            else
                *p = 15;
            p++;
        }
    }

    for (;;) {
        io_hlt();
    }
}
