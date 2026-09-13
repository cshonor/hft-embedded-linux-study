# Ch6 demos — 进程模型

本目录的 8 个 `c6_*.c` 与本章 10 篇笔记一一对应，**每个都在 Compiler Explorer（gcc 13.3.0，x86-64）上真实编译 + 运行过**，输出原样抄在对应笔记的「实测输出」块里。

## 8 个 demo

| 文件 | 对应节 | 演示什么 | 需要的运行环境 |
|------|--------|----------|----------------|
| `c6_1_process_ctx.c` | 6.1 进程基本概念 | `getpid/getppid/getpgrp/getsid/getuid/getgid` + `getrlimit(RLIMIT_NOFILE/RLIMIT_STACK)` | 无特殊要求 |
| `c6_2_pid_orphan.c` | 6.2 PID 与 PPID | A 段正常父子 + `wait`；B 段中间父先退出，孙子 PPID 被改写成 1 | `fork`（Linux） |
| `c6_3_addr_space.c` | 6.3 虚拟地址空间 | 各类变量地址 + `/proc/self/maps` 前 8 行 | 需 `/proc` |
| `c6_4_vm_demand.c` | 6.4 虚拟内存 | `malloc(128MB)` 三阶段查 `/proc/self/statm` 的 RSS | 需 `/proc` |
| `c6_5_stack_frame.c` | 6.5 栈与栈帧 | 栈增长方向、每帧字节数、子进程里爆栈看死因 | `fork` + `wait` |
| `c6_6_argv.c` | 6.6 命令行参数 | dump `argv`、读 `/proc/self/cmdline`、`fork/execv` 传假 `argv[0]` | `fork` + `execv` |
| `c6_7_environ.c` | 6.7 环境列表 | `getenv/setenv/unsetenv/putenv` + 遍历 `environ` | 无特殊要求 |
| `c6_8_setjmp.c` | 6.8 非局部跳转 | 三层调用链 `longjmp` 回 main、三种变量对比、`sizeof(jmp_buf)` | 无特殊要求 |

## 3 个原书 Listing 的最小复刻

保留下来是为了能直接和书里的代码对照；深度实验用上面的 `c6_*`。

| 文件 | 原书 | 说明 |
|------|------|------|
| `t_getenv.c` | Listing 6-1 | 遍历 `environ` 全量打印，11 行 |
| `setjmp_vars.c` | Listing 6-2 | `nvar` / `rvar` / `vvar` 三种变量跨 `longjmp` 的命运 |
| `mem_segments.c` | Listing 6-3 | text / data / bss / heap / stack 五段地址各打一行 |

## 编译

一次编完 8 个：

```bash
for f in c6_*.c; do gcc -O2 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

单个（以 6.4 为例）：

```bash
gcc -O2 -Wall -Wextra -o c6_4_vm_demand c6_4_vm_demand.c && ./c6_4_vm_demand
```

3 个 Listing 复刻（和书一致，用默认优化级别）：

```bash
cc -Wall -Wextra -o t_getenv t_getenv.c && ./t_getenv
cc -Wall -Wextra -o setjmp_vars setjmp_vars.c && ./setjmp_vars
cc -Wall -Wextra -o mem_segments mem_segments.c && ./mem_segments
```

## 运行

| 命令 | 说明 |
|------|------|
| `./c6_1_process_ctx` | 打印身份与两个 rlimit |
| `./c6_2_pid_orphan` | 正常父子 → 孤儿 |
| `./c6_3_addr_space` | 地址 + maps |
| `./c6_4_vm_demand` | 按需分页 / RSS（会短暂持有 128MB 虚拟地址） |
| `./c6_5_stack_frame` | 帧大小 + 子进程爆栈（父进程存活、正常收尸） |
| `./c6_6_argv -a hello --flag` | 需要参数；不给参数也能跑（走打印分支） |
| `./c6_7_environ` | 环境变量五个实验 |
| `./c6_8_setjmp` | `setjmp` / `longjmp` 全家桶 |

## 两个必须知道的坑

1. **子进程里 `printf` 会丢**：`_exit()` **不 flush** stdio 缓冲。`c6_2` / `c6_5` 里在子进程分支显式调了 `fflush(stdout)`，去掉它就能复现「子进程什么都没打印」。
2. **`fork` 会复制未 flush 的 stdio 缓冲**：所以 `c6_2` 的 `--- A. ---` 会打印**两遍**（父子各一次）。这不是 bug，是经典现象——笔记 6.2 里有解读。

## 6.8 的关键对照

同一份源码，两个优化级别结论不同，必须两个都跑：

```bash
gcc -O0 -Wall -Wextra -o c6_8_O0 c6_8_setjmp.c && ./c6_8_O0
gcc -O2 -Wall -Wextra -o c6_8_O2 c6_8_setjmp.c && ./c6_8_O2
```
