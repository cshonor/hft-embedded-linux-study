# Ch13 demos — File I/O Buffering

```bash
cc -Wall -Wextra -o stdio_buffering stdio_buffering.c
./stdio_buffering
./stdio_buffering > /tmp/ob.txt   # watch when lines appear
./stdio_buffering fflush

cc -Wall -Wextra -o mix_stdio_write mix_stdio_write.c
./mix_stdio_write
./mix_stdio_write fix

cc -Wall -Wextra -D_GNU_SOURCE -o odirect_align odirect_align.c
./odirect_align /tmp/odirect_test.bin
```

| 文件 | 说明 |
|------|------|
| `stdio_buffering.c` | TTY 行缓冲 vs 重定向全缓冲；`fflush` |
| `mix_stdio_write.c` | `printf`+`write` 乱序；`fix` 先 fflush |
| `odirect_align.c` | `O_DIRECT` 对齐失败 → `EINVAL`；成功后 `fsync` |

## 代码示例

```c
#include <stdio.h>
/* Ch13 demo: stdio buffering */
int main(void) {
    printf("Line buffered (no newline yet)");
    fflush(stdout);
    printf(" - flushed\n");
    return 0;
}
```

---
