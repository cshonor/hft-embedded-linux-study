# Ch14 demos — File Systems

在 Linux / WSL 下编译运行（通常不需要 root，除 `mount_bind_demo`）。

```bash
cc -Wall -Wextra -o mini_df mini_df.c
./mini_df
./mini_df /

cc -Wall -Wextra -o print_inode print_inode.c
./print_inode /etc/passwd .
# hard link same FS:
#   echo hi > /tmp/a && ln /tmp/a /tmp/b && ./print_inode /tmp/a /tmp/b

cc -Wall -Wextra -o proc_mounts proc_mounts.c
./proc_mounts

# needs root (or CAP_SYS_ADMIN):
cc -Wall -Wextra -o mount_bind_demo mount_bind_demo.c
sudo ./mount_bind_demo
```

| 文件 | 说明 |
|------|------|
| `mini_df.c` | `statvfs` 打印块/inode 用量（简易 df） |
| `print_inode.c` | `stat` 打印 `st_ino` / `st_dev` / `st_nlink` |
| `proc_mounts.c` | 读 `/proc/self/mounts` |
| `mount_bind_demo.c` | `MS_BIND` 绑定挂载再 `umount`（需特权） |

## 代码示例

```c
#include <stdio.h>
#include <sys/statfs.h>
/* Ch14 demo: statfs */
int main(void) {
    struct statfs fs;
    statfs("/", &fs);
    printf("block size: %ld\n", (long)fs.f_bsize);
    return 0;
}
```

---
