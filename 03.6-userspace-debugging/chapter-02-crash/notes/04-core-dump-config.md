# 2.4 core 文件生成配置（ulimit -c / core_pattern / systemd-coredump）

> 🔴 精读 · 让崩溃留下证据

## 本节要点

core dump 是进程收到**无法处理的信号**（`SIGSEGV`/`SIGABRT`/`SIGBUS`/`SIGFPE`/`SIGILL` 等）而被内核终止时，内核把进程内存镜像写到磁盘的文件。它是崩溃分析的唯一「实物证据」。但默认配置下 core **往往根本不会生成**——`ulimit -c 0`、systemd 接管、setuid 限制等都会让证据消失。本节讲清「怎么确保崩溃时一定留下 core」。

## 什么触发 core dump

进程因信号终止时，内核在清理前把它整个地址空间（代码、数据、堆、栈、各线程栈、寄存器上下文）写成 core 文件。触发条件：

| 信号 | 默认动作 | 会 dump？ |
|------|----------|-----------|
| `SIGSEGV` | 终止 | ✅ 段错误（访问非法地址） |
| `SIGABRT` | 终止 | ✅ `abort()`/断言失败 |
| `SIGBUS` | 终止 | ✅ 总线错误（对齐/映射的硬件错误） |
| `SIGFPE` | 终止 | ✅ 除零/浮点异常 |
| `SIGILL` | 终止 | ✅ 非法指令 |
| `SIGTERM`/`SIGINT` | 终止 | ❌ 正常终止信号，不 dump |

> 只有「程序自身错误」导致的异常终止才 dump；被 `kill` 发 `SIGTERM` 优雅退出是不 dump 的。

## 动手：亲手把六种崩溃跑出来（实测）

上表是「哪些信号会 dump」，但**「它到底怎么死的」不能靠背**。`code/c2_1_crash_types.c` 把六种最常见的崩溃各做一次，每次给一个不同的信号——于是 `139 / 134 / 136` 这几个神秘数字变成可复现的事实，同时你也拿到了「哪种死法会留下什么遗言」。

编译（`-fstack-protector-all` 必须显式加，见 case 5）：

```bash
gcc -g -O0 -Wall -Wextra -fstack-protector-all -o c2_1_crash_types c2_1_crash_types.c
./c2_1_crash_types <1..6>
```

实测（gcc 13.3.0，退出码列是 shell 里 `echo $?` 的真实值）：

| case | 代码在做什么 | 信号 | 退出码 | 实测的「遗言」（stderr） |
|------|-------------|------|--------|------------------------|
| 1 | 解引用 `NULL` 订单指针 | `SIGSEGV(11)` | **139** | `Program terminated with signal SIGSEGV (11)` |
| 2 | 主动 `abort()` | `SIGABRT(6)` | **134** | `Program terminated with signal SIGABRT (6)` |
| 3 | `assert(qty > 0)` 失败 | `SIGABRT(6)` | **134** | `c2_1_crash_types.c:59: crash_assert: Assertion 'qty > 0' failed.` |
| 4 | 成交量为 0 时算均价 | `SIGFPE(8)` | **136** | `Program terminated with signal SIGFPE (8)` |
| 5 | `strcpy` 写穿 `char sym[8]` | `SIGABRT(6)` | **134** | `*** stack smashing detected ***: terminated` |
| 6 | 无限递归爆栈 | `SIGSEGV(11)` | **139** | `Program terminated with signal SIGSEGV (11)` |

三条从实测里读出来的结论：

1. **退出码 = 128 + 信号号，这是崩溃分诊的第一把钥匙**。不用 gdb、不用 core，光 `echo $?` 就能把「怎么死的」分成三堆：**139 = 访问了非法内存**、**134 = 程序主动放弃（abort / 断言 / 栈保护）**、**136 = 算术异常**。生产环境脚本里第一件事就该打这个数。
2. **同样是 134，遗言完全不同**。case 2（`abort()`）什么都不说，case 3（`assert`）告诉你**文件:行号:函数:条件**，case 5（栈保护）告诉你 `*** stack smashing detected ***`。**这三种都进同一个 `SIGABRT`，但排查方向差得远**——一个是设计上主动退出，一个是断言被违反，一个是栈被写坏了（后面往往是缓冲区溢出）。
3. **case 5 是关键的教学点**：`char sym[8]` 里拷进 20 字节，程序**没有立刻崩**——是函数返回时检查金丝雀不匹配，才报 `stack smashing detected` 并 abort。这说明「写坏栈」和「因此崩溃」之间有延迟；**如果没有 `-fstack-protector-all`，这一段可能一路静默写坏调用者的栈**，然后过一会儿在完全无关的地方崩。那才是最难的形态，也是 2.6「分析被破坏的内存」要处理的东西。

```c
/* case 5：栈保护金丝雀怎么被发现 */
static int crash_stack(void)
{
    char sym[8];
    strcpy(sym, "AAPL240621C00150000");   /* 20 字节 > 8，写穿局部数组 */
    g_sink = sym[0];
    return 0;                             /* ← 返回时检查金丝雀：不匹配 → __stack_chk_fail → abort */
}
```

> ⚠️ **本节能实测到哪、实测不到哪，说清楚**：
> - **实测到了**：六种信号的**退出码**、以及 `assert` / 栈保护的**真实错误消息**（这些在任意 Linux/容器里都一样）。
> - **没实测到**：**core 文件本身的生成**。上面那些 `ulimit -c` / `core_pattern` / `systemd-coredump` / `suid_dumpable` 的配置需要在**真实 Linux 主机**上改内核参数并观察落盘结果，本仓库的验证环境（Windows 本地 + 编译服务容器）拿不到 —— 容器里的执行器不允许改 `/proc/sys`，也没有 systemd。这一节的配置命令请按官方文档在你的 Linux 机器上验证。
> - **替代验证**：你可以在自己机器上跑一遍这个 demo，配好 `ulimit -c unlimited` 后确认 case 1 输出 `Segmentation fault (core dumped)`（**多出的 `(core dumped)` 就是成功标志**），再 `gdb ./c2_1_crash_types core` 看 `bt`。这条链路就是下一节 2.5 的内容。

## 第一道闸：ulimit -c

`ulimit -c` 控制 core 文件的**大小上限**，默认常是 `0`（彻底禁用）：

```bash
ulimit -c              # 查看当前限制
# 0                    # ← 0 = 不生成 core！

ulimit -c unlimited    # 解除限制（当前 shell 及它启动的进程）
ulimit -c 1073741824   # 限制 1GB（避免 core 撑爆磁盘）
```

```bash
# 测试：开了 ulimit 后，段错误会生成 core
ulimit -c unlimited
./c2_1_crash_types 1        # 本节 demo：空指针解引用
# Segmentation fault (core dumped)   ← "core dumped" 说明成功（少了 (core dumped) 就说明没落盘）
ls -la core*
# -rw------- 1 user user 245760 Sep 3 17:30 core
```

> ⚠️ `ulimit` 是 **shell 级**的，只对当前 shell 及其子进程生效。systemd 管理的服务要在 unit 里配 `LimitCORE=infinity`，cron/脚本要各自设置。**别以为登录 shell 设了，systemd 服务就继承**。

### systemd 服务如何开 core

```ini
# /etc/systemd/system/matching-engine.service
[Service]
LimitCORE=infinity          # 等价 ulimit -c unlimited
```

## 第二道闸：core_pattern（core 写到哪、叫什么）

`/proc/sys/kernel/core_pattern` 决定 core 的**文件名模板**，甚至能把 core **管道给处理程序**：

```bash
cat /proc/sys/kernel/core_pattern
# |/usr/lib/systemd/systemd-coredump %P %u %g %s %t %c %h %e   ← systemd 接管（现代默认）
# 或
# core                                                        ← 传统：写当前目录 core
```

| 模板变量 | 含义 |
|----------|------|
| `%p` / `%P` | 进程 pid（`%P` 含命名空间信息） |
| `%e` | 可执行文件名 |
| `%s` | 触发信号号 |
| `%t` | dump 时间（unix 秒） |
| `%h` | 主机名 |
| `%u` / `%g` | uid / gid |
| `%c` | 进程的 core 大小软限制 |

```bash
# 自定义：写到专用目录，文件名带 pid + 程序名 + 信号
echo '/var/coredump/core.%e.%p.%s' | sudo tee /proc/sys/kernel/core_pattern
# 崩溃后生成：/var/coredump/core.orderbook.12345.11   （11 = SIGSEGV）

# 是否在文件名加 .pid（老式 core_uses_pid，已被 core_pattern 取代）
cat /proc/sys/kernel/core_uses_pid   # 1 = core.12345，0 = core
```

> `|` 开头 = 管道给程序（systemd-coredump 就是这种）。**`|` 后面的程序崩溃了 core 就没了**，排查「core 不生成」时先确认管道程序健在。

## systemd-coredump：现代发行版的默认接管

现代 Linux 用 systemd 接管 core，不再往当前目录写 `core` 文件，而是压缩存进 `/var/lib/systemd/coredump/`，用 `coredumpctl` 管理：

```bash
coredumpctl list                 # 列出所有 core
# TIME                PID  UID  GID SIG  COREFILE  EXE
# Sep 03 17:30:00    12345 1000 1000 11   present   /home/u/orderbook

coredumpctl info 12345           # 看某次崩溃详情（信号、栈、二进制路径）

coredumpctl gdb 12345            # 直接用 gdb 打开该 core（最方便！）
# 等价于 gdb /home/u/orderbook /var/lib/systemd/coredump/...

coredumpctl dump 12345 -o /tmp/core.orderbook   # 导出成独立 core 文件
```

> 生产机推荐直接用 `coredumpctl gdb`，不用手动找 core 文件路径；离线分析则 `coredumpctl dump` 导出。

## setuid / 权限导致的「dump 不了」

出于安全，内核对**改了 uid 的程序**默认不 dump（防止 core 泄露敏感信息）：

```bash
cat /proc/sys/fs/suid_dumpable
# 0 = setuid 程序不 dump（默认）
# 1 = dump（core 归触发用户，但权限受限）
# 2 = dump（core 仅 root 可读）

# 进程自身也可设 PR_SET_DUMPABLE=0 主动禁止 dump
# prctl(PR_SET_DUMPABLE, 0)   ← 有些程序故意屏蔽调试/取证
```

如果程序故意 `PR_SET_DUMPABLE=0`，`/proc/<pid>/status` 里会显示 `CoreDumping: 0`，此时连 root 也拿不到 core（除非改程序）。

## core 不生成的排查清单

崩溃了却没 core，按顺序查这四道闸：

| # | 检查点 | 命令 |
|---|--------|------|
| 1 | ulimit 是否 0 | `ulimit -c` |
| 2 | core_pattern 是否 pipe 给失效程序 | `cat /proc/sys/kernel/core_pattern` |
| 3 | 是否 systemd 接管、core 在 coredump 库里 | `coredumpctl list` |
| 4 | 是否 setuid/PR_SET_DUMPABLE 限制 | `cat /proc/sys/fs/suid_dumpable`；`grep CoreDump /proc/<pid>/status` |

## HFT 关联

1. **生产机必须开 core**：交易进程崩溃时，没有 core 就只能靠日志瞎猜。部署基线里强制 `LimitCORE=infinity` + `core_pattern` 指向 `/var/coredump/` 专用分区（避免 core 撑爆根分区导致更大事故）。
2. **core 归档与磁盘策略**：core 可能上百 MB（交易进程内存大），用 `%e.%p.%s` 命名 + 定时清理脚本，或走 systemd-coredump 的压缩存储 + 配额。
3. **dump 不掉 = 安全特性在生效**：若程序是 setuid 或被 `PR_SET_DUMPABLE=0` 屏蔽，core 拿不到是**设计使然**，排查时要先排除这条，别误以为配置坏了。
4. **保留带符号的二进制**：core 是内存快照，没有配套的调试符号二进制，加载了也回溯不到源码行（见 2.5），两者必须成对归档。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 进程收到 `SIGTERM` 会 core dump 吗？为什么？

> 不会。core dump 只发生在「程序自身错误」导致的异常终止（SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL 等）——这些信号的默认处置是终止且触发 dump。SIGTERM 是外部请求的正常终止信号，默认动作只是终止，不 dump。

**Q2:** 为什么登录 shell 里 `ulimit -c unlimited` 了，systemd 服务崩溃还是没 core？

> `ulimit` 只作用于当前 shell 及其子进程，systemd 服务由 systemd（PID 1）直接启动，不经过你的登录 shell，继承不到。systemd 服务必须在 unit 里显式配 `LimitCORE=infinity`。

**Q3:** `core_pattern` 以 `|` 开头是什么意思？有什么坑？

> `|` 表示把 core 内容通过管道传给后面的程序（如 `|/usr/lib/systemd/systemd-coredump ...`），由该程序负责存储，而不是直接写文件。坑：若这个管道程序本身不可用/崩溃，core 就静默丢失——排查「core 不生成」时必须确认管道目标程序健在。

**Q4:** setuid 程序默认不 dump，是出于什么考虑？

> 安全。setuid 程序可能接触敏感数据（如 passwd、sudo），core 里包含完整内存镜像，若被普通用户 dump 走可能泄露特权信息。所以内核默认 `fs.suid_dumpable=0` 禁止 setuid 程序 dump，或限制 core 的读取权限。

**Q5:** core 文件和带调试符号的二进制为什么必须「成对」保存？

> core 是纯内存快照，本身不含符号信息；要把地址翻译回函数名/源码行，必须加载**崩溃时那一版**的二进制（带 `-g` 调试信息）。二进制版本对不上（改了代码重新编译），地址就错位，回溯全是垃圾。所以 CI 归档 release 时必须同时留符号。

**Q6:** 实测里退出码 134 有三种完全不同的成因，为什么必须区分？各自的「遗言」是什么？

> 因为排查方向完全不同。三种 134 分别是：① `abort()` —— **设计上主动退出**（代码里就有这句，查调用它的条件即可）；② `assert` 失败 —— **不变量被违反**，遗言带 `文件:行号:函数: Assertion 'x' failed`，直接指向出问题的断言；③ 栈保护金丝雀不匹配 —— `*** stack smashing detected ***: terminated`，**说明栈被写坏了**（通常是缓冲区溢出），要顺着「谁写穿了哪个局部数组」查。单看 `134` 只能确定「程序主动放弃」，必须看 stderr 才分得清。

**Q7:** 为什么 case 5 里 `strcpy` 写穿 `char sym[8]` 之后程序没有立刻崩？

> 因为越界写的目标是**自己栈帧里的相邻字节**（保存的寄存器、金丝雀），那块内存物理上是可写的，CPU 不会报错。真正让它变成 `SIGABRT` 的是**函数返回时的金丝雀检查**：编译器在序言里把一份随机值（canary）存进栈帧，返回前比对——`strcpy` 写穿了它，比对失败就调 `__stack_chk_fail` 并 abort。所以「写坏栈」和「因此崩溃」之间有延迟；如果没开栈保护（`-fstack-protector-all`），可能一路静默写坏调用者的栈，过一会儿在无关的地方崩——那是最难查的形态（见 2.6）。

</details>

## 交叉引用

- [2.5 加载 core 回溯](05-load-core-backtrace.md)
- [2.6 深入内存分析](06-analyze-corrupted-memory.md)
- [2.1 gdb 入门与调试信息](01-gdb-intro-build.md)
- [03.6 模块导读](../../README.md)
