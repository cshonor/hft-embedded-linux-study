# Ch9 demos — Process Credentials

```bash
cc -Wall -Wextra -o print_credentials print_credentials.c
./print_credentials

# optional: observe setuid path
cc -Wall -Wextra -o seteuid_drop_restore seteuid_drop_restore.c
sudo chown root:root seteuid_drop_restore
sudo chmod u+s seteuid_drop_restore
./seteuid_drop_restore
```

| 文件 | 说明 |
|------|------|
| `print_credentials.c` | `getresuid`/`getresgid` + 补充组 |
| `seteuid_drop_restore.c` | Saved-ID 支持的降权 / 再提权 |
