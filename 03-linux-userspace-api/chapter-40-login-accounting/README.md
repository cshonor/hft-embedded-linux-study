# TLPI 第 40 章 — Login Accounting

**优先级**：🟠（审计、who/last、会话可见性）  
**前置**：[Ch39 Capabilities](../chapter-39-capabilities/notes.md) · [Ch34 会话](../chapter-34-process-groups-sessions/notes.md)  
**后置**：[Ch41 共享库](../chapter-41-shared-libraries/notes.md)

---

## 小节目录

- [40.1 –40.2 文件](./notes/40.1-section-40-1.md)
- [40.3 `struct utmp`](./notes/40.3-struct-utmp.md)
- [40.4 读取](./notes/40.4-read-op.md)
- [40.5 更新](./notes/40.5-update.md)
- [40.6 现代注意](./notes/40.6-section-40-6.md)

---

## 章节目标


utmp/wtmp/btmp；`struct utmp` 与 `ut_type`；遍历 API；更新由谁做；systemd/容器注意点；简易 `who`。

---


---

## 易错清单


1. utmp ≠ 进程列表；一用户可多条会话  
2. wtmp 含大量 DEAD/BOOT 历史  
3. 用 API，别当文本读  
4. 并发写须锁  
5. `ut_user` 空 ≠ 一定无终端槽  

---


---

## 实验清单


1. 简易 `who`  
2. `utmpname(wtmp)` 扫历史  
3. 找 `BOOT_TIME`  
4. （选）ssh 登录前后 utmp 变化  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | utmp=当前；wtmp=历史；btmp=失败 |
| 2 | 关心 `USER_PROCESS` / `BOOT_TIME` |
| 3 | setutent → getutent → endutent |
| 4 | 会话记录，不是 ps |
| 5 | 写入留给 login/sshd |

---


---

## 参考


- Kerrisk · TLPI Ch40  
- `man 5 utmp` · `man 3 getutent` · `man 3 utmpname`


---

## 代码示例

```c
#include <stdio.h>
#include <utmp.h>
#include <utmpx.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Ch40 登录记账 — utmp/wtmp + last 命令原理。
 * 演示读取 /var/run/utmp 获取当前登录用户。
 * 编译: gcc -o ch40_demo ch40_demo.c */

int main(void) {
    /* utmpx: 读取当前登录记录 */
    setutxent();  /* 打开/重置 utmp 文件 */

    printf("Currently logged in users:\n");
    printf("%-12s %-8s %-16s %-24s\n",
           "USER", "TTY", "HOST", "LOGIN TIME");

    struct utmpx *entry;
    while ((entry = getutxent()) != NULL) {
        /* 只显示用户进程登录记录 */
        if (entry->ut_type == USER_PROCESS) {
            char timebuf[32];
            struct tm *tm = localtime(&entry->ut_tv.tv_sec);
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

            printf("%-12s %-8s %-16s %-24s\n",
                   entry->ut_user,
                   entry->ut_line,
                   entry->ut_host,
                   timebuf);
        }
    }

    endutxent();

    /* 写入 utmp 记录 (通常由 login 程序做) */
    printf("\nNote: utmp is normally written by login(1) and login(1)\n");
    printf("wtmp (/var/log/wtmp) keeps historical login records\n");
    printf("Use 'last' command to read wtmp\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
