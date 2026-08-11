# TLPI 第 40 章 — Login Accounting

> 对应目录：`chapter-40-login-accounting/`  
> 书名原文：**Login Accounting**  
> ⚠️ **utmp/wtmp 是二进制会话账本，不是进程表。** `who`←utmp；`last`←wtmp。用 `setutent`/`getutent`，勿当文本打开。

**优先级**：🟠（审计、who/last、会话可见性）  
**前置**：[Ch39 Capabilities](../chapter-39-capabilities/notes.md) · [Ch34 会话](../chapter-34-process-groups-sessions/notes.md)  
**后置**：[Ch41 共享库](../chapter-41-shared-libraries/notes.md)

---

## 章节目标

utmp/wtmp/btmp；`struct utmp` 与 `ut_type`；遍历 API；更新由谁做；systemd/容器注意点；简易 `who`。

---

## 40.1–40.2 文件

| 文件 | 作用 | 典型路径 | 命令 |
|------|------|----------|------|
| **utmp** | 当前会话 | `/var/run/utmp` 或 `/run/utmp` | `who`/`w` |
| **wtmp** | 登录/登出历史 | `/var/log/wtmp` | `last` |
| **btmp** | 失败登录 | `/var/log/btmp` | `lastb` |

二进制顺序记录；改文件用标准 API + 锁。

---

## 40.3 `struct utmp`

关键字段：`ut_type` · `ut_pid` · `ut_line` · `ut_user` · `ut_host` · `ut_tv` · `ut_session`…

| `ut_type` | 含义 |
|-----------|------|
| `USER_PROCESS` | 有效用户登录（who 主要关心） |
| `DEAD_PROCESS` | 会话结束（写入 wtmp） |
| `BOOT_TIME` | 系统启动 |
| `LOGIN_PROCESS` / `INIT_PROCESS` | getty/init 等 |
| `EMPTY` | 空槽 |

---

## 40.4 读取

```c
setutent();
while ((ut = getutent()) != NULL) { /* ... */ }
endutent();

utmpname("/var/log/wtmp");   /* 再 setutent 读历史 */
```

`getutid` / `getutline`：按条件找。

Demo：[`code/who_utmp.c`](./code/who_utmp.c)

---

## 40.5 更新

通常由 `login`/`sshd`/agetty 维护；自行写须加锁、定位、同步追加 wtmp。写错会搞乱 `who`。

---

## 40.6 现代注意

systemd-logind 管会话；容器常无完整 utmp；wtmp 会被 logrotate 切段。也有 `utmpx` 接口（可移植变体）。

| 命令 | 数据源 |
|------|--------|
| `who` | utmp + `USER_PROCESS` |
| `last` | wtmp |
| `w` | utmp + `/proc` 等 |

---

## 易错清单

1. utmp ≠ 进程列表；一用户可多条会话  
2. wtmp 含大量 DEAD/BOOT 历史  
3. 用 API，别当文本读  
4. 并发写须锁  
5. `ut_user` 空 ≠ 一定无终端槽  

---

## 实验清单

1. 简易 `who`  
2. `utmpname(wtmp)` 扫历史  
3. 找 `BOOT_TIME`  
4. （选）ssh 登录前后 utmp 变化  

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

## 参考

- Kerrisk · TLPI Ch40  
- `man 5 utmp` · `man 3 getutent` · `man 3 utmpname`
