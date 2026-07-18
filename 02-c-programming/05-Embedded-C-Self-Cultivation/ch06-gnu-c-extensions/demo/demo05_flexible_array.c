#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct msg {
    uint32_t len;
    uint8_t  data[];
};

struct msg *msg_create(uint32_t payload_len, const void *payload)
{
    struct msg *m = malloc(sizeof(*m) + payload_len);
    if (!m)
        return NULL;
    m->len = payload_len;
    if (payload && payload_len)
        memcpy(m->data, payload, payload_len);
    return m;
}

int main(void)
{
    const char *text = "hello-flex-array";
    struct msg *m = msg_create(strlen(text) + 1, text);

    printf("sizeof(struct msg)=%zu  total alloc=%zu\n",
           sizeof(struct msg), sizeof(struct msg) + m->len);
    printf("len=%u data=\"%s\"\n", m->len, (char *)m->data);

    free(m);
    return 0;
}
