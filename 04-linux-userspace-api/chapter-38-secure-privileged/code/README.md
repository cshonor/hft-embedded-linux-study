# Ch38 demos — Secure privileged programs

Privilege drop/restore demos live under Ch9 (need setuid-root binary):

```bash
# ../chapter-09-process-credentials/code/
# seteuid_drop_restore.c  setuid_permanent_drop.c
```

```bash
cc -Wall -Wextra -o open_fstat_safe open_fstat_safe.c
./open_fstat_safe /etc/passwd

cc -Wall -Wextra -o no_system_exec no_system_exec.c
./no_system_exec
```

| 文件 | 说明 |
|------|------|
| `open_fstat_safe.c` | `open` + `fstat`（反 TOCTOU 骨架） |
| `no_system_exec.c` | 绝对路径 `execve`，不用 `system` |
