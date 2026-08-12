# Ch5 demos

```bash
cc -Wall -Wextra -o dup_share_offset dup_share_offset.c && ./dup_share_offset
cc -Wall -Wextra -o pread_demo pread_demo.c && ./pread_demo
cc -Wall -Wextra -o o_excl_create o_excl_create.c && ./o_excl_create
cc -Wall -Wextra -o fcntl_nonblock fcntl_nonblock.c && ./fcntl_nonblock
```

| 文件 | 演示 |
|------|------|
| `dup_share_offset.c` | `dup` 共享文件偏移 |
| `pread_demo.c` | `pread` 不改当前偏移 |
| `o_excl_create.c` | `O_CREAT\|O_EXCL` 原子创建 |
| `fcntl_nonblock.c` | 管道上 `fcntl` 设 `O_NONBLOCK` |

## 代码示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
/* Ch5 code demo: dup 共享文件偏移量 */
int main(void) {
    int fd = open("/tmp/dup_demo.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "hello", 5);

    int fd2 = dup(fd);  /* fd2 和 fd 共享文件偏移量 */
    lseek(fd, 0, SEEK_SET);

    char buf[8];
    read(fd2, buf, 5);  /* fd2 读到 fd 写的内容 */
    buf[5] = '\0';
    printf("dup read: %s\n", buf);
    close(fd);
    close(fd2);
    return 0;
}
```

---
