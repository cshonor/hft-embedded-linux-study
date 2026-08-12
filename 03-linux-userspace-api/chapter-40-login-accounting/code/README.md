# Ch40 demos — Login accounting

```bash
cc -Wall -Wextra -o who_utmp who_utmp.c
./who_utmp
./who_utmp wtmp          # if /var/log/wtmp readable
./who_utmp boot          # BOOT_TIME lines from wtmp
```

| 文件 | 说明 |
|------|------|
| `who_utmp.c` | 遍历 utmp/wtmp，打印 USER_PROCESS / BOOT_TIME |

## 代码示例

```c
#include <stdio.h>
#include <utmpx.h>
/* Ch40 demo: getutxent */
int main(void) {
    struct utmpx *e;
    setutxent();
    while ((e = getutxent()))
        if (e->ut_type == USER_PROCESS)
            printf("%s on %s\n", e->ut_user, e->ut_line);
    endutxent();
    return 0;
}
```

---
