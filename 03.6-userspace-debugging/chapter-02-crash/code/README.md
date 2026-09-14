# Ch2 demos — 崩溃类

Ch2 的工具是 gdb 和 coredump，但**「拿起 gdb 之前」**有两件事必须先搞清：**它是怎么死的**、**调用栈长什么样**。这两份 demo 就是干这个的，都在 Compiler Explorer（gcc 13.3.0，x86-64）上真跑过。

> ⚠️ **本目录实测得到什么、实测不到什么**：
> - ✅ **实测到了**：六种信号的真实退出码（139/134/136）、`assert` 与栈保护金丝雀的真实错误消息、进程内回溯的**真实帧数与地址**。
> - ❌ **没实测**：gdb 的交互式命令、core 文件的生成与加载。这两者需要真实 Linux 主机（gdb + `/proc/sys` 可写 + systemd）。本仓库的验证环境（Windows 本地 + 编译服务容器）用不了 —— 容器里不能改内核参数、没有 systemd、也没有 gdb。2.4 / 2.5 里的 gdb 会话与 `core_pattern` 配置请在你的 Linux 机器上验证。
> - 🔁 **替代验证**：`c2_2_backtrace.c` 用 glibc 的 `backtrace()` 在**进程内**打出调用栈 —— 这是 `bt` 的底层机制，可以用它看到「真实回溯长什么样」，不必依赖 gdb。

## 两份 demo

| 文件 | 对应节 | 演示什么 |
|------|--------|----------|
| `c2_1_crash_types.c` | 2.2 / 2.4 / 2.6 | **六种崩溃，六个信号**：`1`=空指针(SIGSEGV/139)、`2`=abort(SIGABRT/134)、`3`=assert(SIGABRT/134)、`4`=除零(SIGFPE/136)、`5`=写穿栈数组(SIGABRT/134 + `*** stack smashing detected ***`)、`6`=无限递归爆栈(SIGSEGV/139) |
| `c2_2_backtrace.c` | 2.3 / 2.7 | **进程内真实回溯**：`main → layer_a → layer_b → layer_c → layer_d → 崩溃`，SIGSEGV 处理器用 `backtrace()` 打印调用栈。★ 关键实验：`-O0` 得 **10 帧**、`-O2` 得 **6 帧**（`layer_a~d` 被内联进 `main`） |

## 编译

```bash
# c2_1：-fstack-protector-all 必须显式加，否则 case 5 不会变成 SIGABRT
gcc -g -O0 -Wall -Wextra -fstack-protector-all -o c2_1_crash_types c2_1_crash_types.c

# c2_2：-rdynamic 让动态符号表导出 main/_start（static 函数仍只有地址，见坑 3）
gcc -g -O0 -rdynamic -Wall -Wextra -o c2_2_bt_O0 c2_2_backtrace.c
gcc -g -O2 -rdynamic -Wall -Wextra -o c2_2_bt_O2 c2_2_backtrace.c
```

## 运行与期望结果（均为实测）

| 命令 | 信号 | 退出码 | 关键输出 |
|------|------|--------|----------|
| `./c2_1_crash_types 1` | SIGSEGV | **139** | 只打出 `case 1: 准备解引用 NULL` |
| `./c2_1_crash_types 2` | SIGABRT | **134** | 只打出 `case 2: 主动 abort()` |
| `./c2_1_crash_types 3` | SIGABRT | **134** | `c2_1_crash_types.c:59: crash_assert: Assertion 'qty > 0' failed.` |
| `./c2_1_crash_types 4` | SIGFPE | **136** | 只打出 `case 4: 算均价而成交量为 0` |
| `./c2_1_crash_types 5` | SIGABRT | **134** | `*** stack smashing detected ***: terminated` |
| `./c2_1_crash_types 6` | SIGSEGV | **139** | 只打出 `case 6: 无限递归…` |
| `./c2_2_bt_O0` | SIGSEGV | **139** | 捕获信号 11，**10 帧**（含 `layer_d/c/b/a` 各一帧） |
| `./c2_2_bt_O2` | SIGSEGV | **139** | 捕获信号 11，**6 帧**（`layer_a~d` 合成 `main+0x85` 一帧） |

## 四个必须知道的坑

1. **退出码 = 128 + 信号号**：`139 = 128+11 (SIGSEGV)`、`134 = 128+6 (SIGABRT)`、`136 = 128+8 (SIGFPE)`。这是崩溃分诊的第一把钥匙：**不用 gdb、不用 core，`echo $?` 就能把「怎么死的」分成三堆**。
2. **`-fstack-protector-all` 不能省**：默认的 `-fstack-protector-strong` 未必给 `char sym[8]` 配金丝雀。不加的话 case 5 可能静默写坏栈、或过一会儿在别处崩——**那才是最难的形态**，反而学不到东西。
3. **`backtrace_symbols` 只认 `.dynsym`**：`-rdynamic` 只导出全局符号，所以 `main`/`_start` 有名字、`static` 函数（`layer_*`、`on_crash`）只有裸地址。gdb 读的是完整的 `.symtab`，所以它的 `bt` 更好看——这是工具差异，不是代码问题。
4. **信号处理函数里只能调 async-signal-safe 函数**：`printf`（拿 stdio 锁）和 `backtrace_symbols`（`malloc`）都可能死锁。本 demo 用 `write()` + `backtrace_symbols_fd()`（`_fd` 版不分配内存）。另外收尾用 `_exit()` 而不是 `exit()`，避免跑 atexit / 刷 stdio。
