#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

struct normal {
    uint8_t  type;
    uint32_t value;
    uint16_t flags;
};

struct packed {
    uint8_t  type;
    uint32_t value;
    uint16_t flags;
} __attribute__((packed));

#define OFF(S, m) offsetof(S, m)

static void dump_offsets(const char *title, size_t total)
{
    printf("\n=== %s (sizeof=%zu) ===\n", title, total);
}

int main(void)
{
    dump_offsets("struct normal", sizeof(struct normal));
    printf("  type   offset=%zu\n", OFF(struct normal, type));
    printf("  value  offset=%zu\n", OFF(struct normal, value));
    printf("  flags  offset=%zu\n", OFF(struct normal, flags));

    dump_offsets("struct packed", sizeof(struct packed));
    printf("  type   offset=%zu\n", OFF(struct packed, type));
    printf("  value  offset=%zu\n", OFF(struct packed, value));
    printf("  flags  offset=%zu\n", OFF(struct packed, flags));

    struct normal n = { .type = 1, .value = 0x12345678, .flags = 0xABCD };
    struct normal *p = &n;
    printf("\np->value=0x%x via ->  (*p).value=0x%x\n", p->value, (*p).value);
    (void)p;
    return 0;
}
