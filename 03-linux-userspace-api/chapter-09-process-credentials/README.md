# TLPI 第 09 章 — Process Credentials

**优先级**：🔴（权限检查 / setuid / 安全铺垫）
**前置**：[Ch8 Users and Groups](../chapter-08-users-and-groups/README.md)（账号库）· [Ch24 fork](../chapter-24-process-creation/README.md) · [Ch27 exec](../chapter-27-program-execution/README.md)
**后置**：[Ch10 Times and Dates](../chapter-10-time/README.md) · [Ch38 Secure Privileged Programs](../chapter-38-secure-privileged/README.md) · [Ch39 Capabilities](../chapter-39-capabilities/README.md)

---

## 小节目录

- [9.1 Real User ID and Real Group ID](notes/9.1-real-uid-gid.md) —— 五类凭证总表；`uid=0` 为什么不等于特权；`/proc/self/status` 的 `Uid:` 是**4 列**；**亲手 `unshare(CLONE_NEWUSER)`**
- [9.2 Effective User ID and Effective Group ID](notes/9.2-effective-uid-gid.md) —— 权限检查看 EUID；`setuid()` 的 EINVAL / EPERM / 0 三个分支；no-op 陷阱
- [9.3 Set-User-ID and Set-Group-ID Programs](notes/9.3-set-user-id.md) —— exec 改什么不改什么；**四条让 SUID 位失效的路**（含 man 里没写的那条）；写文件会清 SUID 位
- [9.4 Saved Set-User-ID and Saved Set-Group-ID](notes/9.4-saved-id.md) —— saved 为什么存在；**临时降权 vs 永久降权**；为什么 `setuid` 不能用来做临时降权
- [9.5 File-System User ID and File-System Group ID](notes/9.5-fs-uid-gid.md) —— **Linux 上文件权限看 fsuid，不是 euid**；`setfsuid()` 成功与失败返回值相同
- [9.6 Supplementary Group IDs](notes/9.6-supplementary-groups.md) —— 读补充组的惯用法；`getgroups` 的 EINVAL **不是 ERANGE**；`setgroups` 的**两道独立闸门**；内核会排序
- [9.7 Retrieving and Modifying Process Credentials](notes/9.7-api.md) —— 接口总表（9.7.1–9.7.4）+ **原书 Listing 9-1**（`proccred/idshow.c`，9.7.5）
- [9.8 Summary](notes/9.8-summary.md) —— 五类凭证 / 权限检查用哪个 ID / `set*id` 语义 / `fork`+`exec` 传递 四张总表
- [9.9 Exercises](notes/9.9-exercises.md) —— 原书练习 9-1 ~ 9-5，答案逐格按 `kernel/sys.c` 推导 + 两个可运行验证器

> 九节的划分与 TLPI 原书一致（9.1 real / 9.2 effective / 9.3 set-user-ID 程序 / 9.4 saved / 9.5 filesystem / 9.6 补充组 / 9.7 接口与例子 / 9.8 Summary / 9.9 Exercises）。
>
> ⚠️ **原书 9.7 内部有 5 个编号子节**（9.7.1 读改 R/E/S · 9.7.2 读改 fs ID · 9.7.3 读改补充组 · 9.7.4 修改凭证调用总表 · 9.7.5 Example: Displaying Process Credentials）——本模块**把它们收在同一篇里**用 `###` 分节，不再拆文件。
>
> ⚠️ **本章官方源码只有 1 个文件**：`proccred/idshow.c`（= **Listing 9-1**，对应 9.7.5）。第 9 章的其余节在原书里没有配套程序。

---

## 章节目标

掌握进程**当前持有**的全套 UID/GID；理解 **set-user-ID / set-group-ID** 程序；分清特权 / 非特权下各 `set*id` 的规则与返回码；知道 Linux 独有的 **filesystem ID** 及其历史来源；为 Ch38（安全特权程序）与 Ch39（capabilities）打底。

内核侧对应：这些 ID 都存在 `struct cred` 里，通过 `current_cred()` 访问。

---

## 五类凭证速查

| # | 类 | 用途 | 读 | 写 |
|---|----|------|----|----|
| 1 | **Real** uid/gid | 拥有者；信号权限；资源记账 | `getuid()` / `getresuid()` | `setuid` / `setresuid` |
| 2 | **Effective** uid/gid | 共享资源（IPC）权限；**「特权」判据** | `geteuid()` / `getresuid()` | `seteuid` / `setreuid` / `setresuid` / `setuid` |
| 3 | **Saved set** uid/gid | 保存 exec 时的 euid，供来回切 | `getresuid()` | `setreuid` / `setresuid` / `setuid`（特权时） |
| 4 | **Filesystem** uid/gid（Linux 专有） | **文件**权限检查 | `setfsuid(-1)` 或 `/proc/self/status` 第 4 列 | `setfsuid` / `setfsgid`（已废弃） |
| 5 | **Supplementary** groups | 文件 / IPC 权限的额外集合 | `getgroups()` | `setgroups()` / `initgroups()` |

---

## Ch8 vs Ch9

| | Ch8 Users and Groups | Ch9 Process Credentials |
|--|----------------------|-------------------------|
| 焦点 | `/etc/passwd` 等**账户库**（文件 + 查询 + 认证） | 进程**运行时**的 UID/GID 集合 |
| 关键对象 | `struct passwd` / `struct group` / `struct spwd` | R / E / S / F + 补充组 |
| 典型 API | `getpwnam` / `getgrgid` / `crypt` | `getresuid` / `setresuid` / `setgroups` |
| 补充组 | 只讲**文件侧**（写在 `/etc/group` 里） | 讲 **API**（`getgroups` / `setgroups`），§9.6 |
| 不讲 | setuid 程序、运行时权限切换 | 账户文件的字段细节 |

---

## 本章 10 个 demo

所有 demo 都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04 容器）上**真实编译 + 运行**过，输出原样抄在各节笔记的「实测输出」块里。

| 文件 | 节 | 演示什么 |
|------|----|----------|
| `code/c9_1_all_creds.c` | 9.1 | 五类凭证的**两条独立读法**（API vs `/proc`）+ `uid_map` / `CapEff` |
| `code/c9_7_user_ns.c` | 9.1 | 亲手 `unshare(CLONE_NEWUSER)`：`overflowuid` 65534、`gid_map` 写得进 `uid_map` 写不进 |
| `code/c9_4_setid_probe.c` | 9.2 / 9.7 | 15 条 `set*id` 调用的返回码实测表（uid / gid 两族） |
| `code/c9_6_suid_bit.c` | 9.3 | 写文件清 SUID 位（7 步）+ 扫 SUID 文件 + `execve` 一个 SUID 文件 |
| `code/c9_5_drop_privileges.c` | 9.4 | `show` / `temp` / `perm` 三种降权模式 |
| `code/c9_2_fsuid.c` | 9.5 | fsuid 两读法 + `setfsuid` 的静默失败 |
| `code/c9_3_groups.c` | 9.6 | `getgroups` 两个 EINVAL 分支 + `setgroups` 两道闸门 + 内核排序 |
| `code/idshow.c` | 9.7 | **原书 Listing 9-1**（`proccred/idshow.c`）的逐行复刻 |
| `code/ex9_1_id_state.c` | 9.9 | 练习 9-1 的可执行验证器（含初值自检） |
| `code/ex9_3_my_initgroups.c` | 9.9 | 练习 9-3：自己实现 `initgroups()` |

完整索引与编译命令见 [`code/README.md`](code/README.md)。

---

## 与后续章节

| 章 | 关联 |
|----|------|
| Ch27 Program Execution | `execve` 的完整语义；set-user-ID 位就在那里生效 |
| Ch38 Secure Privileged Programs | 降权顺序、不可信输入的边界、SUID 程序的写法 |
| Ch39 Capabilities | 用 file capability 替代 set-user-ID；`CapEff` 的完整解释 |
| Ch40 Login Accounting | `utmp` / `wtmp` 记录「谁登录了」——RUID 的另一个用途 |
| Ch15 File Attributes | 权限位本身（`chmod` 的 `s` 位怎么显示） |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | **文件**权限检查看 **fsuid / fsgid / 补充组**；**不是** EUID（`credentials(7)` 明写） |
| 2 | 「特权」的 TLPI 判据是 `EUID == 0`；**内核的真实判据是 capability**（`CAP_SETUID` 等） |
| 3 | setuid 位：`exec` 后 `EUID = 文件属主`，`RUID 不变`，**`Saved ← 新 EUID`（无条件）** |
| 4 | Saved-ID 的唯一用途：让 set-user-ID 程序能 `seteuid` **来回切** |
| 5 | `setuid`(特权) 改 **R+E+S**（不可逆）；非特权只改 E。**临时降权必须用 `seteuid`** |
| 6 | `-1` = 不改该项；任何 euid 变更都会**同步 fsuid** |
| 7 | **`EINVAL` 排在 `EPERM` 之前**：uid 无映射 → EINVAL；uid 合法但不许改 → EPERM |
| 8 | `setfsuid` 成功与失败**返回值相同**，且不设 `errno` → 只能回读判断 |
| 9 | 降权顺序：**`setgroups(0,NULL)` → `setgid(t)` → `setresuid(t,t,t)` → 回读验证** |
| 10 | `getgroups` 的「缓冲不够」是 **`EINVAL`**，不是 `ERANGE` |

---

## 参考

- Kerrisk · TLPI Ch9 Process Credentials
- `man 7 credentials` · `man 2 setuid` · `man 2 setreuid` · `man 2 setresuid` · `man 2 setfsuid` · `man 2 getgroups` · `man 3 initgroups` · `man 7 user_namespaces` · `man 5 proc_pid_status`
- Linux v6.6 源码：`kernel/sys.c`（`set*uid` / `set*fsuid` / `getresuid`）· `kernel/groups.c`（`getgroups` / `setgroups` / `in_group_p`）· `fs/exec.c`（`bprm_fill_uid`）
- 原书源码：`proccred/idshow.c`（**Listing 9-1**）

---

## 代码示例

本章的 demo 都在 [`code/`](code/README.md) 目录下，每个都可独立编译。最常用的一个入口是「打印全部凭证」：

```bash
gcc -O0 -Wall -Wextra -o c9_1_all_creds code/c9_1_all_creds.c && ./c9_1_all_creds
```

而本章唯一一个**原书自带的程序**是 Listing 9-1（`code/idshow.c`），它需要 Ch8 的 `ugid_functions.c` 一起编译：

```bash
gcc -O0 -Wall -Wextra -o idshow code/idshow.c ../chapter-08-users-and-groups/code/ugid_functions.c
./idshow
```
