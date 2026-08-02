# Ch45 demos — System V IPC keys / msgget

在 Linux / WSL 下于本目录执行：

```bash
# touch a stable path for ftok (do not delete while using the key)
touch /tmp/ch45-ftok-token

gcc -Wall -Wextra -o sysv_key_demo sysv_key_demo.c
./sysv_key_demo /tmp/ch45-ftok-token

# inspect / clean leftovers if needed
ipcs -q
# ipcrm -q <msqid>
```

| 文件 | 说明 |
|------|------|
| `sysv_key_demo.c` | `ftok` + `msgget(CREAT\|EXCL)` + `IPC_RMID` |
