# Ch9 demos — Process Credentials

```bash
cc -Wall -Wextra -o print_credentials print_credentials.c
./print_credentials

# setuid-root demos (temporary vs permanent drop)
cc -Wall -Wextra -o seteuid_drop_restore seteuid_drop_restore.c
cc -Wall -Wextra -o setuid_permanent_drop setuid_permanent_drop.c
sudo chown root:root seteuid_drop_restore setuid_permanent_drop
sudo chmod u+s seteuid_drop_restore setuid_permanent_drop
./seteuid_drop_restore
./setuid_permanent_drop
```

| 文件 | 说明 |
|------|------|
| `print_credentials.c` | `getresuid`/`getresgid` + 补充组 |
| `seteuid_drop_restore.c` | Saved-ID 临时降权 / 再提权 |
| `setuid_permanent_drop.c` | `setuid(getuid())` 永久丢弃特权 |
