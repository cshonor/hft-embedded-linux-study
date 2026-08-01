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
