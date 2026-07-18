#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MsgFlex {
    int len;
    char buf[]; /* C99 柔性数组，不计入 sizeof */
};

struct MsgPtr {
    int len;
    char *buf;
};

static void demo_flex(void)
{
    const char *payload = "flexible array: one malloc, contiguous";
    size_t cap = strlen(payload) + 1;
    struct MsgFlex *m = malloc(sizeof(struct MsgFlex) + cap);

    if (!m)
        return;
    m->len = (int)cap;
    memcpy(m->buf, payload, cap);
    printf("Flex: sizeof(MsgFlex)=%zu payload@=%p buf@=%p\n",
           sizeof(struct MsgFlex), (void *)m, (void *)m->buf);
    printf("  %s\n", m->buf);
    free(m);
}

static void demo_ptr(void)
{
    const char *payload = "pointer member: two mallocs, fragmented";
    struct MsgPtr *m = malloc(sizeof *m);
    size_t cap = strlen(payload) + 1;

    if (!m)
        return;
    m->len = (int)cap;
    m->buf = malloc(cap);
    memcpy(m->buf, payload, cap);
    printf("Ptr:  sizeof(MsgPtr)=%zu header@=%p buf@=%p\n",
           sizeof(struct MsgPtr), (void *)m, (void *)m->buf);
    printf("  %s\n", m->buf);
    free(m->buf);
    free(m);
}

int main(void)
{
    demo_flex();
    demo_ptr();
    return 0;
}
