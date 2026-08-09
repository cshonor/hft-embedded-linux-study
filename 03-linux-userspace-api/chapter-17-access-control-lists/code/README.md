# Ch17 demos — POSIX ACL

需要：支持 ACL 的文件系统 + 开发包（Debian/Ubuntu: `libacl1-dev`）。

```bash
cc -Wall -Wextra -o print_acl print_acl.c -lacl
./print_acl /tmp/tlpi_acl_demo.txt
# or after: setfacl -m u:$USER:rw /tmp/some_file && ./print_acl /tmp/some_file

cc -Wall -Wextra -o set_named_user_acl set_named_user_acl.c -lacl
./set_named_user_acl /tmp/tlpi_acl_demo.txt
getfacl /tmp/tlpi_acl_demo.txt
ls -l /tmp/tlpi_acl_demo.txt    # note trailing '+' and group column = mask
```

| 文件 | 说明 |
|------|------|
| `print_acl.c` | 遍历 Access ACL，打印 ACE（简易 getfacl） |
| `set_named_user_acl.c` | 写扩展 ACL：命名用户 + MASK |
