# TLPI 第 08 章 — Users and Groups

**优先级**：🟡→🔴（嵌入式权限 / 安全铺垫）
**前置**：[Ch6 Processes](../chapter-06-processes/README.md)（进程的 uid/gid 从哪来）
**后置**：[Ch9 进程凭证](../chapter-09-process-credentials/README.md) · [Ch15 文件属性/权限](../chapter-15-file-attributes/README.md)

---

## 小节目录

- [8.1 The Password File：`/etc/passwd`](notes/8.1-passwd-file.md) —— 7 字段；为什么必须全局可读
- [8.2 The Shadow Password File：`/etc/shadow`](notes/8.2-shadow-file.md) —— 9 字段；`!` / `*` / 空串的精确语义
- [8.3 The Group File：`/etc/group`](notes/8.3-group-file.md) —— 4 字段；主组 vs 附属组；`gr_mem` 的空组陷阱
- [8.4 Retrieving User and Group Information](notes/8.4-retrieving-info.md) —— 查询 API；`errno` 范式；静态缓冲；`_r` 版本与 `ERANGE`；**原书 Listing 8-1**
- [8.5 Password Encryption and User Authentication](notes/8.5-password-encryption.md) —— `crypt()`；盐 = 算法标识；`*0` / `*1` 与别名漏洞；**原书 Listing 8-2**
- [8.6 Summary](notes/8.6-summary.md) —— 三文件 / 三结构体 / 本章实测数据汇总
- [8.7 Exercises](notes/8.7-exercises.md) —— 四道题，题 1 与题 4 有实测参考实现

> 七节的划分与 TLPI 原书一致（8.1 passwd / 8.2 shadow / 8.3 group / 8.4 查询 API / 8.5 `crypt` 与认证 / 8.6 Summary / 8.7 Exercises）。原书这几节内部**没有编号子节**，所以每节的内容都收在各自一篇里。
>
> ⚠️ **附属组的 API**（`getgroups` / `setgroups` / `initgroups`）**不在本章** —— 属 [Ch9 §9.6](../chapter-09-process-credentials/README.md)。本章只讲「主组写在哪、附属组写在哪」这个**文件侧**事实。

---

## 章节目标

理解 Linux 的用户/组**数据库**：`/etc/passwd` / `/etc/shadow` / `/etc/group` 三个文本文件的字段语义与权限要求；掌握查询这三套库的 API 家族（含 `_r` 可重入版与静态缓冲的生命周期）；掌握 `crypt()` 认证链路**以及它的三个陷阱**；为 Ch9 的「进程运行时凭证」铺垫。

---

## Ch8 vs Ch9 速查

| | Ch8 Users and Groups | Ch9 Process Credentials |
|--|----------------------|-------------------------|
| 焦点 | 账户**数据库**（文件 + 查询 + 认证） | 进程**运行时** UID/GID 集合 |
| 关键对象 | `struct passwd` / `struct group` / `struct spwd` | RUID / EUID / SUID / FSUID / 附属组 |
| 核心 API | `getpwnam` / `getgrgid` / `getspnam` / `crypt` | `setuid` / `seteuid` / `setreuid` / `setgroups` |
| 不讲 | setuid 程序、权限切换 | 账户文件字段细节 |
| 附属组 | **只讲文件侧**（写在 `/etc/group` 的名单里） | **讲 API**（`getgroups` / `setgroups` / `initgroups`）§9.6 |

---

## 易错清单

全部有实测支撑（细节见各节与 [8.6](notes/8.6-summary.md)）：

1. **`/etc/passwd` 是 644 不是"配置不严"**，是设计使然（`ls -l` 要把 uid 翻译成名字）；反过来 **`/etc/shadow` 是 644 就是确凿的错误**（`shadow(5)`："must not be readable by regular users"）。
2. **`pw_passwd` 只是占位符 `x`**；拿它直接 `crypt()` 会得到 `*0`。认证前**必须**用 `sp_pwdp` 覆盖它。
3. **非 `_r` 版本返回静态缓冲**，`getpwent` / `getpwnam` / `getpwuid` **共用同一块**；「读一批存数组」直接是 bug。
4. **地址不可假设**：`fgetpwent` 连续三次返回同一地址，但 `getpwnam("ce")` 与 `getpwuid(10240)`（同一用户）地址**不同**。
5. **`errno` 范式有条件**：后端起不开时（`/etc/group` 不存在）`errno` 会是 `ENOENT`，无法与「真的出错」区分。man 明确列了 `0 / ENOENT / EBADF / ESRCH / EPERM` 都可能。
6. **`_r` 版「不存在」= `s == 0` 且 `*result == NULL`**，只看 `s` 不够；且 `_r` **不动 `errno`**，别再对它用 `errno` 范式。
7. **空数字字段（shadow）= `-1`**，不是 `0`（`getspnam(3)`：`putspent()` 把 `-1` 写成空串）。`sp_max == 0` 与 `sp_max < 0` 语义完全不同。
8. **空组的 `gr_mem` 不是 NULL**，而是「首元素为 NULL 的数组」。写 `if (gr->gr_mem != NULL)` 会走错分支。
9. **`crypt()` 失败可能返回 `*0` 而不是 `NULL`**（libxcrypt 的实现与 man 的描述不一致），必须两种都判。
10. **`crypt()` 的返回值绝不能当盐再喂回去**：会得到 `*1`，两边变成同一块内存 → `strcmp` 恒等 → **任何口令都"通过"**。
11. **DES 只吃口令前 8 字符**（不足补 0），盐只有 2 字符 / 4096 种 —— 两个限制合起来判了 DES-crypt 的死刑。
12. **glibc ≥ 2.28 用 `_DEFAULT_SOURCE`**，不是老教程的 `_XOPEN_SOURCE`；glibc 2.39 本身不带 `crypt()`，靠 libxcrypt，**要链 `-lcrypt`**。
13. **本章不涉及 RUID/EUID/SUID 与 `setuid()`** —— 那是 [Ch9](../chapter-09-process-credentials/README.md)。
14. **`getgrgid` / `getpwuid` 的路径编译期写死**（`nss/files-XXX.c:42` 的 `#define DATAFILE "/etc/" DATABASE`）；想读自己造的文件只能用 `fgetpwent` / `fgetgrent` / `fgetspent`。

---

## 章节链路

```text
Ch6  进程是谁在跑（地址空间 / 凭证的宿主）
  → Ch8  系统里有哪些用户/组（三个文件 + 查询 API + crypt 认证）
  → Ch9  这个进程此刻用哪套 UID/GID（凭证 + setuid + 附属组 API）
  → Ch15 文件 rwx 如何用 UID/GID 判定
  → Ch38 特权程序怎么写才不被打穿
  → Ch40 utmp/wtmp 登录记账
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 精简镜像常没有 `/etc/group`、甚至没有 `/etc/shadow`；「uid → 名字」的代码必须能接受 `NULL`。BusyBox 的 `crypt` 支持面远小于 libxcrypt（常只有 DES / MD5），跨机构建时算法要统一 |
| HFT | 热路径绝不查账户库（接 LDAP/SSSD 后是网络往返）；日志记**数字 uid**；交易节点通常关掉口令登录，此时应让 `/etc/shadow` 里**没有任何合法 `crypt()` 结果** |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | UID 0 = root；主组在 `/etc/passwd` 第 4 字段，附属组在 `/etc/group` 第 4 字段 |
| 2 | passwd 必须 644；shadow 必须 600/640；`x` 是占位符 |
| 3 | `!` = 锁定（**保留原密文**）；`*` = 无口令登录；空串 = 无口令直接登 |
| 4 | get\*nam / get\*uid 返回静态缓冲；调前 `errno = 0`；`_r` 版返回错误号且「`s==0` + `res==NULL`」= 不存在 |
| 5 | shadow 空数字字段 = `-1`；空组 `gr_mem` 是「首元素 NULL 的数组」 |
| 6 | `crypt(key, salt)` 的盐 = 算法标识 + 轮数 + 随机前缀；产出 `$id$salt$hashed` |
| 7 | 摘要固定长：MD5 22 / SHA-256 43 / SHA-512 86；默认 5000 轮 |
| 8 | `crypt()` 失败是 `*0`；`crypt()` 的返回值不能当盐再喂回（`*1` → 认证绕过） |
| 9 | 账户库 = Ch8；进程凭证 = Ch9；附属组的 API 在 Ch9 §9.6 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 8 — Users and Groups**（Listing 8-1 p.159、Listing 8-2 p.164）
- `man 5 passwd` · `man 5 shadow`（shadow-utils）· `man 5 group` · `man 3 getpwnam` · `man 3 getgrnam` · `man 3 getspnam` · `man 3 crypt`
- [OUTLINE](../OUTLINE.md) · [Ch9](../chapter-09-process-credentials/README.md) · [Ch15](../chapter-15-file-attributes/README.md)

---

## 代码示例

本章 `code/` 下有 16 个文件（14 个自写 demo + 原书 Listing 8-1 / 8-2 的复刻），全部在 Compiler Explorer（gcc 13.3.0，x86-64）上真实编译 + 运行过，输出原样抄在对应笔记的「实测输出」块里。完整索引见 [`code/README.md`](code/README.md)。

**不需要 `-lcrypt` 的**：

```bash
for f in c8_1_*.c c8_2_*.c c8_3_*.c c8_4_*.c c8_5_*.c ex8_1_*.c t_getpwent.c t_getpwnam_r.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

**需要 `-lcrypt` 的**：

```bash
for f in c8_6_*.c c8_7_*.c c8_8_*.c c8_9_*.c check_password.c; do
    gcc -O0 -Wall -Wextra -lcrypt -o "${f%.c}" "$f" || echo "FAIL $f"
done
# c8_7 是计时程序，建议开优化
gcc -O2 -Wall -Wextra -lcrypt -o c8_7_crypt_cost c8_7_crypt_cost.c
```

**Listing 8-1 要三个文件一起编**：

```bash
gcc -O0 -Wall -Wextra -o t_ugid t_ugid.c ugid_functions.c && ./t_ugid
```

**一句话结论**（都是这台评测机上的实测值，不是书上抄的）：

- `/etc/passwd` **只有 1 条记录**（`ce`，且末行无换行符），但进程 `uid=0` —— `getpwuid(0) -> NULL`（`errno=0`）
- `/etc/shadow` 是 **0644** —— 按 `shadow(5)` 原文这是**确凿的配置错误**；`/etc/group` **根本不存在**（`getgrent` 遍历到 0 个组）
- `getgrgid(10240)` 返回 `NULL` 且 **`errno=2 (ENOENT)`** —— 「`errno` 范式」在这里会给出错误结论
- 单核吞吐：`$1$`（MD5-crypt）**8910 次/秒** ↔ `$6$`（SHA-512）**656** ↔ `$2b$08`（bcrypt）**17**
- `bad_verify("definitely-wrong", stored_ptr)` 返回 **1** —— 错口令"认证通过"，因为 `crypt()` 的返回值被当盐喂回、`strcmp` 两边同址
