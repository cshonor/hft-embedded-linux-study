# Ch9 demos — Process Credentials

本目录的 **10 个 `.c` 文件**与本章 9 篇笔记对应，**每个都在 Compiler Explorer（gcc 13.3.0，x86-64，Ubuntu 24.04 容器）上真实编译 + 运行过**，输出原样抄在对应笔记的「实测输出」块里。

> ⚠️ **CE 只接受单文件**，所以 `idshow.c`（Listing 9-1）那一组是用 `gen_ce_single.py` 把 `ugid_functions.h` + `idshow.c` + `ugid_functions.c` 拼成一个 `.c` 编译验证的（拼接顺序必须是「头文件 → 用它的 `.c` → 实现」，否则 `userNameFromId` 会被隐式声明成 `int()` 而报 `conflicting types`）。本目录里这几个文件仍是分开的正常形态。

## 7 个自写 demo

| 文件 | 对应节 | 演示什么 | 环境依赖 |
|------|--------|----------|----------|
| `c9_1_all_creds.c` | 9.1 | 五类凭证的**两条独立读法**（API vs `/proc/self/status`）；`uid_map` / `CapEff` / `NoNewPrivs`；两个 `sysconf` 上限 | 读 `/proc` |
| `c9_2_fsuid.c` | 9.5 | fsuid 的两个读法**一致**；`setfsuid(10240)` **静默失败**（返回值与成功相同、errno 不动）；内核四条放行条件 | `<sys/fsuid.h>` |
| `c9_3_groups.c` | 9.6 | 读补充组的惯用法；`getgroups` 的**负 size** 分支（**EINVAL 不是 ERANGE**）；`setgroups` 的**两道闸门**；`initgroups` 与它同因同果；`getgrouplist` 走 NSS | 读 `/proc/self/setgroups` |
| `c9_4_setid_probe.c` | 9.2 / 9.7 | **15 条** `set*id` 调用的返回码实测表（uid 8 条 + gid 7 条）；「按规则应当」与「实测」并排；末尾验证「身份变了没有」 | 无 |
| `c9_5_drop_privileges.c` | 9.4 | `show` / `temp` / `perm` 三种模式；临时降权 vs 永久降权；**失败时如实打印「本环境做不到」** | 真机需要 root |
| `c9_6_suid_bit.c` | 9.3 | **7 步**观察「写文件清 SUID 位、只读不清」；扫 5 个目录里的 SUID 文件；把扫到的那个 `execve` 一遍并解释为什么不提权 | 需要能建 `/tmp` 文件 |
| `c9_7_user_ns.c` | 9.1 | 亲手 `unshare(CLONE_NEWUSER)`：写映射前 uid = **65534**（`overflowuid`）；`gid_map` 写得进、`uid_map` 写不进（**EPERM 由 `fclose` 暴露**） | `CLONE_NEWUSER` 可用的内核 |

## 1 个原书程序（复刻）

| 文件 | 原书 | 说明 |
|------|------|------|
| `idshow.c` | **Listing 9-1**（`proccred/idshow.c`） | 打印 R/E/S/F + 补充组。用 `getresuid()` 一次拿三个，用 `fsuid = setfsuid(0);` 这个怪接口读 fsuid（原书注释保留）。**本章唯一的官方源码文件** |

> 原书 Ch9 **只发布了这一个程序**（`all_files_by_chapter.html` 里 `proccred/` 目录只有 `idshow.c` 一项，标注 "Listing 9-1"）。其余的 set\*id 例子都是正文里的代码片段，没有独立文件。

## 2 个练习实现 / 验证器

| 文件 | 用途 |
|------|------|
| `ex9_1_id_state.c` | 练习 9-1 的**可执行验证器**：先用 `argv[1]` 选 a..e，**自检初值**（`real != 0 && effective == 0 && saved == 0`），不符就明说「本环境做不了」并给出真机四条命令；符合则打印「实测 / 期望 / 依据 / 是否一致」四行 |
| `ex9_3_my_initgroups.c` | 练习 9-3：自己实现 `initgroups()`。用 **`fgetgrent(FILE *)`** 扫一个**可以自己指定**的组文件（容器里没有 `/etc/group`，所以先造一个合成文件），再与 glibc 的 `getgrouplist()` 对照 |

## 编译

**全部 7 个自写 demo + 2 个练习**（自带 `_GNU_SOURCE`，不需要额外旗标）：

```bash
for f in c9_1_all_creds c9_2_fsuid c9_3_groups c9_4_setid_probe \
         c9_5_drop_privileges c9_6_suid_bit c9_7_user_ns \
         ex9_1_id_state ex9_3_my_initgroups; do
    gcc -O0 -Wall -Wextra -o "$f" "$f.c" || echo "FAIL $f"
done
```

**Listing 9-1 那组要三个文件一起编**（依赖 Ch8 Listing 8-1 的 `ugid_functions`）：

```bash
gcc -O0 -Wall -Wextra -o idshow idshow.c ../chapter-08-users-and-groups/code/ugid_functions.c
```

## 运行

| 命令 | 说明 |
|------|------|
| `./c9_1_all_creds` | 两条读法对照 + `uid_map` + `CapEff` + `sysconf` 上限 |
| `./c9_7_user_ns` | `unshare` 造新 ns：65534 / 两个 map 的写入差异（**要看子进程那 10 步**） |
| `./c9_4_setid_probe` | 15 条 `set*id` 的返回码表 |
| `./c9_6_suid_bit` | 7 步清位实验 + 扫 SUID 文件 + `execve` 实测 |
| `./c9_5_drop_privileges show` | 只打印 R/E/S（本容器唯一能走的模式） |
| `./c9_5_drop_privileges temp 1001` | 临时降权再回切（**真机 + root**） |
| `./c9_5_drop_privileges perm 1001` | 永久降权（**真机 + root**，之后 `setuid(0)` 必然失败） |
| `./c9_2_fsuid` | fsuid 两读法 + 静默失败 |
| `./c9_3_groups` | 补充组的读 / 写闸门 / 排序 |
| `./idshow` | 原书 Listing 9-1（本容器输出全 `???` + 数字 0，原因见笔记） |
| `./ex9_1_id_state a` | 练习 9-1 的 (a)（依次 a..e；**每题要重启进程**） |
| `./ex9_3_my_initgroups` | 练习 9-3（默认 `alice 1000`；也可 `./ex9_3_my_initgroups alice 1000`） |

### 需要 root / SUID 才能看全的

```bash
# 永久降权（真机）
sudo useradd -m bob && id bob           # 拿到 bob 的 uid，假设 1001
sudo ./c9_5_drop_privileges temp 1001
sudo ./c9_5_drop_privileges perm 1001

# 练习 9-1：造出「SUID-root 程序被 uid 1000 的用户执行」的初始状态
sudo chown root ex9_1_id_state && sudo chmod u+s ex9_1_id_state
sudo -u '#1000' ./ex9_1_id_state a      # 依次 a..e

# 练习 9-3：用 SUID 拿到 CAP_SETGID，让 setgroups 真的生效
sudo chown root ex9_3_my_initgroups && sudo chmod u+s ex9_3_my_initgroups
./ex9_3_my_initgroups alice 1000
```

## 6 个必须知道的坑

1. **`set*id` 返回 0 不等于「身份变了」。** `setuid(getuid())` 是 no-op 却返回 0；容器里 `setuid(10240)` 返回 `-1/22` 却**什么都没改**。`c9_4` 末尾专门有一栏「试完一圈，身份变了没有？」来钉这件事——**要判断降权效果必须回读 `getresuid()`**。

2. **`EINVAL` 排在 `EPERM` 之前。** 容器里 `uid_map` 只有 `0 113 1` 一行，所以 `setuid(10240)` 得到 **EINVAL(22)**；真机上是 **EPERM(1)**。原因是 `kernel/sys.c:620-622` 的 `make_kuid` + `uid_valid` 检查在 `ns_capable_setid`（`:630`）**之前**。见到 22 先去看 `/proc/self/uid_map`。

3. **`setfsuid()` 成功与失败返回值完全相同，而且不设 `errno`。** `c9_2` 里 `setfsuid(10240)` 返回 `0`、`errno=0`，看起来完美，实际**没生效**。唯一可靠的读法是 `setfsuid((uid_t) -1)`（`-1` 永远不合法，一定不改状态）或直接读 `/proc/self/status` 的 `Uid:` **第 4 列**。

4. **写文件会清掉 SUID/SGID 位，只读不会。** `c9_6` 的 7 步实测：`chmod 04755` → `mode=4755` → 写 1 字节 → **`mode=0755`**；再 `chmod 04755` → **只读**打开 → 仍是 `4755`。所以构建流程里 `chmod u+s` 必须放在所有写操作**之后**。

5. **`getgroups(-1, NULL)` 会在编译期被 gcc 抓出来。** glibc 给 `getgroups` 标了 `__attribute__((access(write_only, 2, 1)))`，直接写负字面量会触发 `-Wstringop-overflow`。`c9_3` 用 `volatile int neg_size = -1;` 转一手，**目的不是骗过检查，而是让内核那一层的 `EINVAL` 也能被演示到**（该告警本身是好东西）。

6. **`/proc` 里那些文件的写入，失败是在 `fclose()` 时暴露的。** `c9_7` 写 `uid_map` 时 `fputs=1`（写缓冲成功）但 **`fclose=-1 errno=1 (EPERM)`**。用 stdio 写 `/proc/pid/{uid_map,gid_map,setgroups}` 时**必须检查 `fclose` 的返回值**，只看 `fputs`/`fprintf` 会误判成功。

## 已知告警（都是故意的）

- `c9_6_suid_bit.c` 的 `execve` 子进程会往 stderr 打一行 `getpwuid failed` —— 那是被 `execve` 的 `ssh-keysign` 自己打的诊断（它拿不到自己的身份），**不是本程序的输出**。父进程那行「本程序**没变** —— 它既没有 set-user-ID 提权，也没有崩」才是要看的结论。
- `c9_5_drop_privileges` 的 `temp` / `perm` 两次运行**都以退出码 1 结束**（`main` 把 `do_temp` / `do_perm` 的返回值直接 `return` 出去）。这是有意的：失败要能被脚本捕获。
- `c9_3_groups.c` 里那两行「本环境只有 N 个补充组，构造不出（需要 >= 2 个组）」是**如实报告环境不足**，不是没写完。本仓库所有 demo 的统一约定是：**做不成的实验要写清楚做不成，不打印假的成功输出**。
