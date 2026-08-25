#include <stddef.h>
#include <stdio.h>

/*
 * offsetof：成员相对结构体起始的字节数。
 * malloc 头块是裸内存，没有这个 struct；这里用它验证「手算偏移」有没有被 padding 骗到。
 * 我们的 mm.c 用 8 字节 header（64 位 size_t），所以 payload 应从 8 开始。
 */

struct block_meta {
    size_t header;
    char payload[8];
};

struct s1 { char a; int b; char c; };
struct s2 { int b; char a; char c; };

int main(void)
{
    if (offsetof(struct block_meta, header) != 0)
        return 1;
    if (offsetof(struct block_meta, payload) != sizeof(size_t)) {
        fprintf(stderr, "payload off=%zu\n", offsetof(struct block_meta, payload));
        return 1;
    }
    /* s1：char+pad+int+char+pad；s2：int + 两个 char 可以挤在同一对齐单元里。 */
    if (sizeof(struct s1) <= sizeof(struct s2)) {
        fprintf(stderr, "s1=%zu s2=%zu (s1 should waste more padding)\n",
                sizeof(struct s1), sizeof(struct s2));
        return 1;
    }
    printf("offsetof payload=%zu  s1=%zu s2=%zu\n",
           offsetof(struct block_meta, payload),
           sizeof(struct s1), sizeof(struct s2));
    return 0;
}
