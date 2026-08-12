# Ch8 demos

```bash
cc -Wall -Wextra -o users_groups users_groups.c && ./users_groups | head

# needs crypt; on some systems link -lcrypt; usually needs root to read shadow
cc -Wall -Wextra -o check_password check_password.c -lcrypt
sudo ./check_password "$USER"
```

| 文件 | 说明 |
|------|------|
| `users_groups.c` | 遍历 passwd / group |
| `check_password.c` | `getspnam` + `crypt` 校验 |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
/* Ch8 demo: getpwuid */
int main(void) {
    struct passwd *pw = getpwuid(getuid());
    printf("user: %s, uid: %u, gid: %u\n",
           pw->pw_name, (unsigned)pw->pw_uid, (unsigned)pw->pw_gid);
    return 0;
}
```

---
