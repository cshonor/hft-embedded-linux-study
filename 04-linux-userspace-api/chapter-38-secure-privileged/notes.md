# TLPI 第 38 章 — Writing Secure Privileged Programs

> 对应目录：`chapter-38-secure-privileged/`  
> （勿用 `…-secure-privileged-programs` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 一致）  
> 书名原文：**Writing Secure Privileged Programs**  
> ⚠️ **`seteuid` = 临时降权可恢复；root 下 `setuid(getuid())` = 永久丢权。** 少写 SUID；能用 Capability（[Ch39](../chapter-39-capabilities/notes.md)）则用。禁 `system`/`execvp`+脏 PATH；防 TOCTOU：先 `open` 再 `fstat`。

**优先级**：🔴（SUID、daemon 降权、攻击面）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)  
**后置**：[Ch39 Capabilities](../chapter-39-capabilities/notes.md)

---

## 章节目标

SUID/SGID 安全模型；`setuid` vs `seteuid`；临时/永久丢权；TOCTOU、环境、符号链接、shell 注入；多层准则；衔 Capability。

---

## 38.1 两类特权程序

| 类型 | |
|------|--|
| root 启动的 daemon | 启动 EUID=0，可再降权 |
| SUID/SGID 文件 | exec 后 EUID=文件属主（如 `passwd`） |

凭证细节见 Ch9：RUID / EUID / Saved-UID / FSUID。Saved-UID 支撑临时降权后再提权。

---

## 38.2 丢弃与恢复（核心）

```c
int setuid(uid_t uid);
int seteuid(uid_t euid);
```

| 调用方 | `setuid(uid)` | `seteuid(euid)` |
|--------|---------------|-----------------|
| **EUID==0** | 常把 **R+E+S 全改** → **永久丢 root** | 只改 EUID → 可再 `seteuid(0)`（若 Saved 仍为 0） |
| **EUID≠0**（典型 SUID） | 多等价于只改 EUID | 在 RUID↔Saved 间切 EUID |

### 临时下放（推荐）

```c
seteuid(getuid());   /* 无特权业务 */
/* ... */
seteuid(0);          /* 必要特权操作 */
```

### 永久丢弃

```c
setuid(getuid());    /* 确认不再需要特权 */
```

Demo（需 setuid-root 二进制）：  
[Ch9 `seteuid_drop_restore.c`](../chapter-09-process-credentials/code/seteuid_drop_restore.c) ·  
[Ch9 `setuid_permanent_drop.c`](../chapter-09-process-credentials/code/setuid_permanent_drop.c)

---

## 38.3 安全准则（精要）

1. **尽量别写 SUID**；改权限 / 专用 daemon / **Capability**  
2. **最小特权**：特权代码短；默认无特权跑业务  
3. 不再需要 → **永久** `setuid(getuid())`  
4. **环境**：勿信 PATH/`LD_*`；清空后只注可信变量；用**绝对路径** `exec*`，禁 `execlp`/`execvp`/`system`/`popen`  
5. **禁 shell**：`system`/`popen` → 注入  
6. **TOCTOU**：勿 `stat` 再 `open`；`open`（可加 `O_NOFOLLOW`）+ `fstat(fd)`  
7. 符号链接：可写目录慎放敏感文件  
8. **校验一切输入**；防 `../`  
9. 创建文件：`umask` + 显式 mode + `O_EXCL`；少用世界可写 `/tmp` 存秘密  
10. `RLIMIT_CORE=0` 防敏感 core；信号里不做特权重活  
11. `O_CLOEXEC` / 关多余 fd  
12. 勿信 stdin/stdout/stderr  

Demo：[`code/open_fstat_safe.c`](./code/open_fstat_safe.c) · [`code/no_system_exec.c`](./code/no_system_exec.c)（注释对照）

---

## 易错清单

1. root 下用 `setuid`「临时」降权 → 回不去  
2. `system` + 脏 PATH  
3. `stat`→`open` TOCTOU  
4. 全程持 root 跑复杂逻辑  
5. fork 继承 UID；SUID 行为在 exec 时生效  

---

## 实验清单

1–2. Ch9 临时/永久降权  
3. `open`+`fstat` vs TOCTOU  
4. （选）PATH 劫持对比  
5. `O_NOFOLLOW`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 权限看 EUID；Saved 管恢复 |
| 2 | seteuid 临时；setuid(root) 永久 |
| 3 | 少 SUID → Capability / daemon |
| 4 | 禁 system；绝对路径 exec |
| 5 | open+fstat，防 TOCTOU |
| 6 | 清环境、校验输入、关多余 fd |

---

## 参考

- Kerrisk · TLPI Ch38  
- [Ch9 notes](../chapter-09-process-credentials/notes.md) · `man 7 credentials` · `man 2 seteuid`
