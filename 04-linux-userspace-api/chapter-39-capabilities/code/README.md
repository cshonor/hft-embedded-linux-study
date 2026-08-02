# Ch39 demos — Capabilities

需要：`libcap` 开发包（Debian/Ubuntu: `libcap-dev`）。

```bash
cc -Wall -Wextra -o cap_view cap_view.c -lcap
./cap_view

# optional (needs privileges to set file caps):
# sudo setcap 'cap_net_bind_service=ep' ./some_bin
# getcap ./some_bin
# grep Cap /proc/$$/status
```

| 文件 | 说明 |
|------|------|
| `cap_view.c` | libcap 打印 Effective/Permitted/Inheritable |
