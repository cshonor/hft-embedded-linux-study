# Ch37 demos — Daemons

```bash
cc -Wall -Wextra -o become_daemon.o -c become_daemon.c
cc -Wall -Wextra -o mini_daemon mini_daemon.c become_daemon.o
./mini_daemon
# check: ps -o pid,ppid,sid,tty,cmd -p $(cat /tmp/tlpi_mini_daemon.pid)
# stop:  kill $(cat /tmp/tlpi_mini_daemon.pid)

cc -Wall -Wextra -o t_syslog t_syslog.c
./t_syslog
# journalctl /var/log/syslog depending on distro

cc -Wall -Wextra -o daemon_sighup daemon_sighup.c become_daemon.o
./daemon_sighup
# kill -HUP $(cat /tmp/tlpi_daemon_sighup.pid)
# kill $(cat /tmp/tlpi_daemon_sighup.pid)
```

| 文件 | 说明 |
|------|------|
| `become_daemon.c` / `.h` | TLPI 风格 `becomeDaemon()` |
| `mini_daemon.c` | 守护化后写 PID 文件睡等 SIGTERM |
| `t_syslog.c` | openlog/syslog |
| `daemon_sighup.c` | SIGHUP 置位 + 主循环「重载」 |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
/* Ch37 demo: daemonize */
int main(void) {
    if (fork()) return 0;
    setsid();
    umask(0);
    chdir("/");
    close(0); close(1); close(2);
    /* daemon running... */
    return 0;
}
```

---
