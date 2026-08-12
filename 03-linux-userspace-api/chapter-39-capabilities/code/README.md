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

## 代码示例

```c
#include <stdio.h>
#include <linux/capability.h>
/* Ch39 demo: capget (need -lcap) */
int main(void) {
    struct __user_cap_header_struct h = {.version=_LINUX_CAPABILITY_VERSION_3, .pid=0};
    struct __user_cap_data_struct d[2];
    if (capget(&h, d) == 0)
        printf("effective caps: 0x%x\n", d[0].effective);
    return 0;
}
```

---
