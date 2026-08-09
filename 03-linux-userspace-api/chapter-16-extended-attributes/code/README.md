# Ch16 demos — Extended Attributes

需要支持 xattr 的文件系统（ext4/tmpfs 等；WSL 通常可用）。

```bash
cc -Wall -Wextra -o xattr_demo xattr_demo.c
./xattr_demo /tmp/tlpi_xattr_demo.txt

# optional CLI check:
# setfattr -n user.comment -v hi /tmp/tlpi_xattr_demo.txt
# getfattr -d /tmp/tlpi_xattr_demo.txt
```

| 文件 | 说明 |
|------|------|
| `xattr_demo.c` | `user.*` 增删改查 + `listxattr` 枚举 |
