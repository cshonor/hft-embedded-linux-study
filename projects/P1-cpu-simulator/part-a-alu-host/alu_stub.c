#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct flags {
    int z, n, c, v;
};

static uint8_t alu_add(uint8_t a, uint8_t b, struct flags *f)
{
    unsigned sum = (unsigned)a + (unsigned)b;
    uint8_t r = (uint8_t)sum;
    f->z = (r == 0);
    f->n = (r >> 7) & 1;
    f->c = (sum > 0xFFu);
    f->v = ((~(a ^ b) & (a ^ r)) >> 7) & 1;
    return r;
}

int main(void)
{
    struct flags f;
    uint8_t r = alu_add(1, 2, &f);
    assert(r == 3 && f.z == 0);
    r = alu_add(200, 100, &f);
    assert(f.c == 1);
    puts("P1 alu_stub OK");
    return 0;
}
