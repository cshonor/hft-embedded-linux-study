# Ch18 demos — Directories and Links

```bash
cc -Wall -Wextra -o list_dir list_dir.c
./list_dir .
./list_dir /tmp

cc -Wall -Wextra -o links_demo links_demo.c
./links_demo /tmp/tlpi_links_demo

cc -Wall -Wextra -o rename_safe_write rename_safe_write.c
./rename_safe_write /tmp/tlpi_safe.txt "hello atomic"

cc -Wall -Wextra -o unlink_open unlink_open.c
./unlink_open /tmp/tlpi_unlink_open.txt
```

| 文件 | 说明 |
|------|------|
| `list_dir.c` | `opendir`/`readdir` + `lstat` 类型 |
| `links_demo.c` | 硬链 vs 软链，inode / nlink |
| `rename_safe_write.c` | 写临时文件再 `rename` |
| `unlink_open.c` | open 后 unlink，仍可读 fd |
