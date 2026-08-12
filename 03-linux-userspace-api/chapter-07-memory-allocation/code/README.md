# Ch7 demos

```bash
cc -Wall -Wextra -o sbrk_probe sbrk_probe.c && ./sbrk_probe
cc -Wall -Wextra -o free_and_sbrk free_and_sbrk.c && ./free_and_sbrk
# optional: numAllocs blockSize freeStep freeMin freeMax
./free_and_sbrk 1000 10240 1 1 999
```

| 文件 | 说明 |
|------|------|
| `sbrk_probe.c` | `sbrk(0)` 查询与扩/缩断点 |
| `free_and_sbrk.c` | Listing 7-1：`free` 后 break 常不降 |

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
/* Ch7 demo: malloc + realloc + free */
int main(void) {
    int *p = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) p[i] = i;
    p = realloc(p, 20 * sizeof(int));
    for (int i = 10; i < 20; i++) p[i] = i;
    printf("p[0]=%d p[19]=%d\n", p[0], p[19]);
    free(p);
    return 0;
}
```

---
