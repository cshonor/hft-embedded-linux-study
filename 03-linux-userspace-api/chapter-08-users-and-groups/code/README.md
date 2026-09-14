# Ch8 demos — 用户与组

本目录的 16 个文件与本章 7 篇笔记对应，**每个都在 Compiler Explorer（gcc 13.3.0，x86-64，Ubuntu 24.04 容器）上真实编译 + 运行过**，输出原样抄在对应笔记的「实测输出」块里。

> ⚠️ **CE 只接受单文件**，所以 `ugid_functions.h` + `ugid_functions.c` + `t_ugid.c` 那组是用 `gen_ce_single.py` 拼成一个 `.c` 编译验证的（只丢掉本地 `#include "ugid_functions.h"`）。本目录里三个文件仍然是分开的正常形态。

## 9 个自写 demo

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|----------|
| `c8_1_passwd_parse.c` | 8.1 | 手工按 `:` 切 7 段并与 `getpwnam()` 逐字段对照；**文件末行无换行符**；`getpwuid(0) -> NULL` | 无 |
| `c8_2_shadow_file.c` | 8.2 | `stat` 出 `mode=0644`；`fork` + 降权实验（如实报告 `EINVAL`）；`fgetspent` 解析合成 shadow，看 `!` / `*` 与空字段 = `-1` | `fork` + `wait` |
| `c8_3_group_file.c` | 8.3 | 真实环境 `/etc/group` 不存在（`ENOENT`）；`fgetgrent` 解析合成 group；**空组的 `gr_mem[0] == NULL`** | 无 |
| `c8_4_lookup_api.c` | 8.4 | `errno` 范式；**静态缓冲铁证**（`fgetpwent` 三次同地址）；`getpwnam` vs `getpwuid` 地址不同 | 无 |
| `c8_5_lookup_r.c` | 8.4 | `getpwnam_r` 成功 / 「不存在」= `s==0 && res==NULL`；**缓冲 8→64 的 `ERANGE` 重试** | 无 |
| `c8_6_crypt_schemes.c` | 8.5 | 7 种盐的密文长度与算法；同盐同口令必同 / 异盐必异；**DES 只吃前 8 字符**；`!` / `*` 锁定哨兵 | `-lcrypt` |
| `c8_7_crypt_cost.c` | 8.5 | 单核吞吐实测（`$1$` / `$5$` / `$6$` / `$2b$08`）+ 一千万词字典换算；bcrypt cost=12 由 cost=8 推算 | `-lcrypt`，建议 `-O2` |
| `c8_8_crypt_traps.c` | 8.5 | 静态缓冲；**别名漏洞**（`bad_verify` 放行错口令、`stored_ptr` 被改写成 `*1`）；正确的 `good_verify` | `-lcrypt` |
| `c8_9_auth_pipeline.c` | 8.5 | 原书 Listing 8-2 的**可自动化等价物**：造合成 shadow → `fgetspent` 读回 → 逐条验证；末尾读真实 shadow 对照 | `-lcrypt` |

## 5 个原书配套文件（复刻）

| 文件 | 原书 | 说明 |
|------|------|------|
| `ugid_functions.c` | **Listing 8-1（p.159）** | 四个「名字 ↔ 数字 ID」转换助手；TLPI 后面十几章都在用 |
| `ugid_functions.h` | Listing 8-1 的头 | 原书说明："This file is **not printed in the book**" |
| `check_password.c` | **Listing 8-2（p.164）** | `getspnam` + `getpass` + `crypt` 的完整认证流程；三处现代 glibc 适配见文件头注释 |
| `t_getpwent.c` | 配套，非 Listing | `getpwent()` 顺序遍历（原书说明："a supplementary file for Chapter 8"） |
| `t_getpwnam_r.c` | 配套，非 Listing | `getpwnam_r()` 单查；**原书那行打的是 `pw_gecos`，不是 `pw_name`** |

## 2 个自写驱动 / 练习

| 文件 | 用途 |
|------|------|
| `t_ugid.c` | 验收 Listing 8-1 四个函数的驱动：数字快捷路径、`(uid_t)-1` → `4294967295`、`NULL` 边界 |
| `ex8_1_uid_info.c` | 8.7 练习 1 的参考实现：UID → 用户名 / 主组 / 附属组，**把三种失败分开** |

## 编译

**不需要 `-lcrypt` 的**：

```bash
for f in c8_1_*.c c8_2_*.c c8_3_*.c c8_4_*.c c8_5_*.c ex8_1_*.c t_getpwent.c t_getpwnam_r.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

**需要 `-lcrypt` 的**（glibc 2.39 已不带 `crypt()`，靠 libxcrypt）：

```bash
for f in c8_6_*.c c8_7_*.c c8_8_*.c c8_9_*.c check_password.c; do
    gcc -O0 -Wall -Wextra -lcrypt -o "${f%.c}" "$f" || echo "FAIL $f"
done
gcc -O2 -Wall -Wextra -lcrypt -o c8_7_crypt_cost c8_7_crypt_cost.c    # 计时程序
```

**Listing 8-1 那组要三个文件一起编**：

```bash
gcc -O0 -Wall -Wextra -o t_ugid t_ugid.c ugid_functions.c && ./t_ugid
```

## 运行

| 命令 | 说明 |
|------|------|
| `./c8_1_passwd_parse` | 手工切分 vs `getpwnam()` 对照 |
| `./c8_2_shadow_file` | 权限位 + 降权实验 + `fgetspent` |
| `./c8_3_group_file` | 真实环境 vs 合成 group 文件 |
| `./c8_4_lookup_api` | `errno` 范式 + 静态缓冲地址证据 |
| `./c8_5_lookup_r` | `_r` 版本 + `ERANGE` 重试 |
| `./c8_6_crypt_schemes` | 7 种盐 / DES 截断 / 锁定哨兵 |
| `./c8_7_crypt_cost` | 单核吞吐表（约 1.6 秒跑完） |
| `./c8_8_crypt_traps` | 别名漏洞复现 + `good_verify` |
| `./c8_9_auth_pipeline` | 端到端认证流水线 |
| `./t_ugid` | Listing 8-1 验收 |
| `./ex8_1_uid_info 10240` / `./ex8_1_uid_info 0` | 练习 1（存在 / 不存在两种情况） |
| `./t_getpwent` | 遍历（本容器只有一行 `ce       10240`） |
| `./t_getpwnam_r ce` | 单查（输出 `Name: Not a real account`，原书打的是 `pw_gecos`） |
| `./check_password` | 交互式；非 tty 时从 stdin 读用户名与口令 |
| `echo -e 'ce\nsecret' \| ./check_password` | 走「口令不匹配」分支（exit 1） |
| `echo -e 'nosuchuser\nx' \| ./check_password` | 走 `fatal()` 分支（`ERROR: couldn't get password record`） |

## 7 个必须知道的坑

1. **`/etc/group` 不存在时 `getgrgid` 会设 `errno=ENOENT`**：它**不是**「系统错误」，而是「后端不可用」（glibc `nss/files-XXX.c:69-85` 的 `internal_setent()` 在 `fopen` 失败时只设 `UNAVAIL`，**不碰 errno**）。照教科书写 `if (errno == 0) 不存在; else 出错;` 会给出错误结论 —— 见 `c8_3` 与 `ex8_1_uid_info`。
2. **`getpwnam` / `getpwuid` 的"没找到"反而会把 `errno` 还原成 0**（`nss/files-XXX.c:130` 的 `__set_errno(saved_errno)`）。所以 `c8_4` 里 `errno=0` 是**真的干净**，`c8_3` 里 `errno=2` 也是**真的** —— 两条路径的差别就是「文件能不能打开」。
3. **别把 `fgetpwent` / `fgetgrent` / `fgetspent` 的返回值存起来**：它们和 `getpwnam` 一样共用静态缓冲。`c8_4` 故意打印三个地址 + 「用 `p1` 再读一次」，就是为了把这件事变成可见的证据。
4. **`/etc/shadow` 是 0644**：这台评测机就是这么配的。所以 `c8_2` 的降权实验**注定失败**（`setuid(10240)` → `EINVAL`，因为容器在 user namespace 里只映射了 uid 0）。程序如实打印失败原因，而不是编一个「权限不足」的假输出 —— **做不成的实验要写清楚做不成**。
5. **`crypt()` 的失败返回值是 `*0`（字符串），不是 `NULL`**：与 `man 3 crypt` 的描述不一致。`c8_6` 里 `$y$` 那一行就是活样本。任何只判 `NULL` 的认证代码都会漏。
6. **`crypt()` 的返回值不能当盐再喂回去**：`c8_8` 复现了完整链条 —— 返回 `*0` → 若那块内存原本是 `*0` 则返回 `*1` 并**就地覆写** → `strcmp` 两边同址 → 恒等成立 → **错口令也通过**。这是功能级安全漏洞。

   > 编译时注意：如果在注释里同时写 `*0/*1` 会触发 `-Wcomment`（`"/*" within comment`）。本目录的注释统一写成 `*0 与 *1`。
7. **原书 `t_getpwnam_r.c` 打的是 `pw_gecos`**：`printf("Name: %s\n", pwd.pw_gecos);` —— 已核对 man7.org 的官方源码，确实如此；`man 3 getpwnam` 的 EXAMPLES 里也是 `pw_gecos`。所以 `./t_getpwnam_r ce` 输出 `Not a real account` 是**预期行为**，不是抄错。

## 已知告警（都是故意的）

- `t_getpwent.c` / `check_password.c` 在 `-Wall -Wextra` 下各报 2 条 `-Wunused-parameter`（`argc` / `argv` 没用到）。**这是原书原样带来的**，为了保持"逐行复刻"的对照价值，不做修饰。
- `c8_7_crypt_cost.c` 必须 `-O2`；用 `-O0` 也能跑，但那测的是 libcrypt 调用序列而不是 `crypt()` 本体，数字会偏。
