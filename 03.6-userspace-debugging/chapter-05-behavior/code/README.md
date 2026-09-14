# Ch5 demos — 行为类

Ch5 的工具是 strace / ltrace，它们都靠 `ptrace` 接管进程。

> ⚠️ **strace / ltrace 在本仓库的验证环境跑不了** —— 编译服务容器既没有 strace 也不允许 `ptrace`，Windows 本地也没有 Linux 环境。（Docker Desktop 已安装但引擎未启动，且 WSL 被安全策略拦截。）所以笔记里的 **strace 输出是「手册格式示意」**。

**但输出的语义可以实测。** 这三份 demo 把 strace 的每一行「翻译」成程序自己能打印的事实，全部在 Compiler Explorer（gcc 13.3.0）上真跑过：

## 三份 demo

| 文件 | 对应节 | 演示什么 |
|------|--------|----------|
| `c5_1_write_buffering.c` | 5.1 / 5.2 | **write 什么时候发生**：`nobuf`/`line`/`buf` 三种 `setvbuf` 模式各跑一次，结尾必然 SIGFPE。★ `buf` 模式（默认，非 tty 时全缓冲）下 **stdout 完全为空**——缓冲区还没刷到内核，进程就死了 |
| `c5_2_syscall_map.c` | 5.1 | **把 C 语句翻译成 strace 行**：自己打印每次 syscall 的返回值与 errno（`open` → fd 3、不存在 → `-1 ENOENT(2)`、关闭的 fd → `-1 EBADF(9)`、`unlink` → 0） |
| `c5_3_read_semantics.c` | 5.1 / 5.2 | **read 的三种返回值**：用 16 字节小缓冲逐轮读 stdin，打印 `=16 / =7（部分读）/ =0（EOF）` |

## 编译

```bash
gcc -g -O0 -Wall -Wextra -o c5_1_write_buffering c5_1_write_buffering.c
gcc -g -O0 -Wall -Wextra -o c5_2_syscall_map      c5_2_syscall_map.c
gcc -g -O0 -Wall -Wextra -o c5_3_read_semantics   c5_3_read_semantics.c
```

## 运行与期望结果（均为实测）

| 命令 | 退出码 | 期望结果 |
|------|--------|----------|
| `./c5_1_write_buffering nobuf` | **136** | `mode=nobuf stdout_isatty=0 ...` + 第 1/2/3 行都在，然后 SIGFPE |
| `./c5_1_write_buffering line` | **136** | 同上（行缓冲，每遇 `\n` 已刷出） |
| `./c5_1_write_buffering buf` | **136** | **stdout 一行都没有** ← 全缓冲，输出随进程消失 |
| `./c5_2_syscall_map` | **0** | 7 组「返回 N / errno=N(名字)」，fd=3、ENOENT=2、EBADF=9 |
| `./c5_3_read_semantics < read_payload.txt` | **0** | `=16 / =16 / =7（部分读）/ =0（EOF）` |

## 输入数据

`read_payload.txt` —— 40 字节的文本（16 + 16 + 7 + `\n`），用来喂 `c5_3_read_semantics`，正好逼出「部分读」和「EOF」。

## 四个必须知道的坑

1. **stdout 接管道/文件时是全缓冲**：`c5_1` 的 `buf` 模式就是靠这个现象教学的。想让输出「死前也留住」，用 `setvbuf(stdout, NULL, _IONBF, 0)` 或每条后 `fflush(stdout)`（`c1_2_shrink_demo.c` 用的是后者）。
2. **strace 里「卡住」的标志是「没有 `=` 的那一行」**：`recvfrom(5, ` 后面空着，因为 syscall 还没返回。`c5_3` 里的 `= -1 EINTR` 是另一种情况——被打断了但**已经返回**，属于要重试的正常路径。
3. **`read` 返回 0 不是错误**：是 EOF。把 0 当失败处理会「读完了却报错」，把 -1 当 EOF 处理会「静默截断」。`c5_3` 的输出里 `= 0` 和 `= -1` 是两回事。
4. **`/dev/full` 在容器里可能不存在**：`c5_2` 的第 6 步（演示 `ENOSPC`）在 CE 上被跳过，会打印「打不开（本环境无 /dev/full），跳过」。在真实 Linux 主机上这一步会给出 `write(/dev/full, "x", 1) = -1 ENOSPC`。
