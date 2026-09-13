# TLPI 第 06 章 — Processes

**优先级**：🔴（后续 fork/信号/多进程的地基）
**前置**：[Ch3 系统编程概念](../chapter-03-system-programming-concepts/README.md) · [Ch4 通用文件 I/O](../chapter-04-file-io-universal/README.md) · [Ch5 深入文件 I/O](../chapter-05-file-io-further/README.md)
**后置**：[Ch7 内存分配](../chapter-07-memory-allocation/README.md) · [Ch8 用户与组](../chapter-08-users-and-groups/README.md) · [Ch24 进程创建](../chapter-24-process-creation/README.md)

---

## 小节目录

- [6.1 进程基本概念：程序、进程与内核的两块账本](notes/6.1-basic-concepts-process.md)
- [6.2 PID 与 PPID：两个 PID，和孤儿是怎么被收养的](notes/6.2-pid-ppid.md)
- [6.3 进程虚拟地址空间：段落在哪，内核怎么记](notes/6.3-address-space-process.md)
- [6.4 虚拟内存管理：地址是假的，物理页是欠着的](notes/6.4-virtual-memory.md)
- [6.5 栈与栈帧：272 字节一帧，30840 层就没了](notes/6.5-stack-frames.md)
- [6.6 命令行参数 `argv`：`argv[0]` 可以是任何字符串](notes/6.6-command-line-args.md)
- [6.7 环境列表：`environ` 的读写与 `putenv` 的两个陷阱](notes/6.7-environment-list.md)
- [6.8 非局部跳转：`setjmp` / `longjmp` 与 `volatile` 陷阱](notes/6.8-setjmp-longjmp.md)
- [6.9 本章总结：进程是什么，内核替它记了什么](notes/6.9-summary.md)
- [6.10 练习：三道题，都能跑](notes/6.10-exercises.md)

---

## 章节目标

本章只回答一个问题：**内核把什么东西打包成了一个「进程」**。拆成四层看：

1. **身份**：PID / PPID / PGID / SID，以及为什么 `getpid()` 和 `gettid()` 在多线程下返回不同的值。（6.1 / 6.2）
2. **地盘**：虚拟地址空间怎么切（text / rodata / data / bss / heap / stack），每一段的边界在内核里由哪个字段记。（6.3 / 6.4 / 6.5）
3. **初始数据**：`argv` 和 `environ` 这两块数据从哪来、谁复制、谁能改、改完影响谁。（6.6 / 6.7）
4. **控制流**：`setjmp` / `longjmp` 跨越栈帧跳转，以及为什么它跟 `volatile` 绑在一起。（6.8）

> ⚠️ **本章不含 `fork()` / `execve()` 正文**。进程的分裂与替换在 Ch24–28。本章讲的是「被 fork 出来的那个东西长什么样」。

---

## 易错清单

1. **`argv[argc]` 一定是 `NULL`**，不是「可能」——它是标准保证的循环终止条件。
2. **显式写 `= 0` 的全局变量进 BSS，不是 DATA**。判据是「初值是否为零常量」，不是「有没有写 `=`」。
3. **BSS 在磁盘上的可执行文件里几乎不占空间**（只占一个大小字段），加载后分配并清零。
4. **`getenv()` 返回的指针不要 `free()`，也不要写**——它直接指向 `environ` 里的字符串。
5. **`putenv()` 不复制字符串**。传局部栈数组 → 函数返回后 `environ` 里就是悬垂指针；**悬垂之后 `getenv` 往往还能命中**，这才是最危险的地方。
6. **`setenv(..., overwrite=0)` 不覆盖已有值**，`putenv` 则无条件覆盖。两者语义不同。
7. **`getenv()` 返回空串 ≠ 返回 `NULL`**：`PATH=` 是「存在但值为空」，`NULL` 才是「不存在」。
8. **`setjmp` 之后的非 `volatile` 局部变量，`longjmp` 回来可能被回滚**（`-O2` 下实测会）。要跨跳转存活的局部变量必须 `volatile`。
9. **`longjmp(env, 0)` 实际返回 `1`**——`val == 0` 被强制成 1（`setjmp/longjmp.c:41` 的 `val ?: 1`）。
10. **`longjmp` 不恢复自动变量的值，也不回滚堆**；它只恢复寄存器 + 栈指针。
11. **栈溢出不是「可检查的返回错误」，是 `SIGSEGV`**——保护页挡住在前一页，进程直接死。
12. **用 `environ`，别依赖 `main` 的第三个参数 `envp`**——POSIX 没规定，用 `environ` 才可移植。
13. 本章**没有 `fork` / `exec`**；别和 Ch24–27 混章。demos 里出现的 `fork` 只是**为了造场景**（造孤儿、改 `argv[0]`），不是本章的教学目标。

---

## 章节链路

```
Ch3–5  fd 表 / 文件描述符（stdin/out/err 已经挂在进程上）
  → Ch6  进程身份 + 地址空间 + argv/environ + setjmp     ← 本章
  → Ch7  堆：brk / mmap / malloc（6.3 的 heap 段接着讲）
  → Ch8  UID / GID（6.1 打印的那几个 id 接着讲）
  → Ch24 fork：复制本章讲的全部东西（地址空间、environ、fd 表、信号处置）
  → Ch27 execve：把 argv/environ 交给新程序（6.6 / 6.7 的收件人）
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 搞清 bss/heap/stack 三段的增长方向与预算，栈是稀缺资源；环境变量当配置用，但少依赖 `putenv`；`-fstack-usage` 能直接量出每帧字节数 |
| HFT | 地址空间是后续 `mmap` / 大页 / `mlock` 的坐标起点；`setjmp` 恢复的是寄存器快照，热路径上别用；RSS 才代表真花钱，虚拟地址不等于内存 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 程序 = 磁盘上的文件；进程 = 运行实例 + 内核里的 `task_struct` |
| 2 | `getpid()` 返回 **tgid**（线程组 ID），`gettid()` 返回**线程 ID**；单线程时两者相等 |
| 3 | 地址空间顺序：text < rodata < data ≈ bss < heap →（向高增长）；stack 在高地址（向低增长） |
| 4 | `argv` / `environ` 都在栈区附近，由内核在 `execve` 时铺好，`mm_struct.arg_start/arg_end/env_start/env_end` 记边界 |
| 5 | `getpid` / `getppid` **永不失败**；孤儿被 `init`（PID 1）收养，PPID 变成 1 |
| 6 | 改环境变量只影响自己和后代，**不影响父 shell** |
| 7 | `setjmp` 首次返回 0；`longjmp(env, v)` 让 `setjmp` 返回 `v`（`v == 0` 时返回 1） |
| 8 | 跨 `longjmp` 存活的局部变量必须 `volatile`，否则可能被优化掉或回滚 |
| 9 | 栈溢出 → `SIGSEGV`，不是 `errno` |
| 10 | `fork` / `exec` 在 Ch24–28，不在本章 |

---

## 参考

- Kerrisk · *The Linux Programming Interface* **Ch6 — Processes**
- `man 2 getpid` · `man 2 getrlimit` · `man 3 setjmp` · `man 3 setenv` · `man 3 getenv` · `man 5 proc`（`/proc/PID/maps`、`/proc/PID/cmdline`、`/proc/PID/statm`）
- 内核源码（v6.6）：`include/linux/sched.h:743`（`task_struct`）· `include/linux/mm_types.h:673`（`mm_struct`）· `kernel/sys.c:958/964/975`（三个 get\*id）· `fs/proc/base.c:358`（cmdline）· `mm/memory.c:4067`（`do_anonymous_page`）
- glibc 2.39：`setjmp/setjmp.h:32/41` · `setjmp/longjmp.c:29/34/41/50-52` · `sysdeps/x86_64/setjmp.S:31`
- [OUTLINE](../OUTLINE.md)

---

## 本章代码

8 个 demo 与 10 篇笔记一一对应，**全部经 Compiler Explorer（gcc 13.3）真实编译并运行过**，输出原样贴在各篇笔记里。源码在 [`code/`](code/README.md)。

| 文件 | 对应节 | 演示什么 |
|------|--------|----------|
| [`c6_1_process_ctx.c`](code/c6_1_process_ctx.c) | 6.1 | `getpid/getppid/getpgrp/getsid/getuid…` + `getrlimit(RLIMIT_NOFILE / RLIMIT_STACK)` |
| [`c6_2_pid_orphan.c`](code/c6_2_pid_orphan.c) | 6.2 | A 段正常父子；B 段人为造孤儿，看 PPID 被改写成 1 |
| [`c6_3_addr_space.c`](code/c6_3_addr_space.c) | 6.3 | 各类变量的地址 + `/proc/self/maps` 前 8 行对照 |
| [`c6_4_vm_demand.c`](code/c6_4_vm_demand.c) | 6.4 | `malloc(128MB)` 在「未触碰 / 全量读 / 写一半」三阶段的 RSS |
| [`c6_5_stack_frame.c`](code/c6_5_stack_frame.c) | 6.5 | 量栈增长方向与每帧字节数；把无上限递归放**子进程**里看它怎么死 |
| [`c6_6_argv.c`](code/c6_6_argv.c) | 6.6 | dump `argv` + 读 `/proc/self/cmdline` + `fork/execv` 传假 `argv[0]` |
| [`c6_7_environ.c`](code/c6_7_environ.c) | 6.7 | `setenv` 的 overwrite 语义 + `putenv` 两个陷阱 + 遍历 `environ` |
| [`c6_8_setjmp.c`](code/c6_8_setjmp.c) | 6.8 | 三层调用链里 `longjmp` 回 main；三种变量对比；`sizeof(jmp_buf)` |

另有 3 个**原书 Listing 的最小复刻**（保留以便和书对照）：

| 文件 | 原书 | 与上面哪个 demo 互补 |
|------|------|----------------------|
| [`t_getenv.c`](code/t_getenv.c) | Listing 6-1 | `c6_7`（加上了 `setenv` / `putenv` 的坑） |
| [`setjmp_vars.c`](code/setjmp_vars.c) | Listing 6-2 | `c6_8`（加上了调用链、`sizeof`、`longjmp(env,0)`） |
| [`mem_segments.c`](code/mem_segments.c) | Listing 6-3 | `c6_3`（加上了 `/proc/self/maps` 交叉验证） |

### 一次编完全部

```bash
cd code
for f in c6_*.c; do gcc -O2 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

### 跑法（含参数的那些）

```bash
./c6_1_process_ctx                      # 身份 + 两个 rlimit
./c6_2_pid_orphan                       # 正常父子 → 孤儿
./c6_3_addr_space                       # 地址 + maps
./c6_4_vm_demand                        # 按需分页 / RSS
./c6_5_stack_frame                      # 帧大小 + 子进程爆栈
./c6_6_argv -a hello --flag             # argv 布局 + cmdline
./c6_7_environ                          # 环境变量五个实验
./c6_8_setjmp                           # setjmp/longjmp 全家桶

# 6.8 的关键对照：同一份源码，两个优化级别结论不同
gcc -O0 -Wall -Wextra -o c6_8_O0 c6_8_setjmp.c && ./c6_8_O0
gcc -O2 -Wall -Wextra -o c6_8_O2 c6_8_setjmp.c && ./c6_8_O2
```

### 本章实测数据一览

| 节 | 命令 | 结论 |
|----|------|------|
| 6.1 | `./c6_1_process_ctx` | `RLIMIT_NOFILE` 只有 **100**（容器里），说明限制值不能硬编码；`RLIMIT_STACK` = **8 MB** |
| 6.2 | `./c6_2_pid_orphan` | 子进程 `ppid` = 父的 pid；中间父退出后孙子 `ppid` 变成 **1** |
| 6.3 | `./c6_3_addr_space` | `main` `0x4010a0` < 字面量 `0x402211` < data/bss `0x4040xx` < heap `0x24ed…` < stack `0x7ffe…`；同一 ELF 在 maps 里占 **5 个映射** |
| 6.4 | `./c6_4_vm_demand` | `malloc(128MB)` 后 RSS **+0.1 MB**；全量读 → **+0.2 MB**；只写一半 → **+64.2 MB** |
| 6.5 | `./c6_5_stack_frame` | **272 字节/帧**；8 MB ÷ 272 ≈ **30840 层**；爆栈是 `SIGSEGV(11)` |
| 6.6 | `./c6_6_argv -a hello --flag` | `argv[0]` 可以显示成任意字符串（`改名了`）而真实路径是 `/app/output.s` |
| 6.7 | `./c6_7_environ` | `setenv(...,0)` 不覆盖、`(...,1)` 覆盖；`putenv` 传局部数组 → 悬垂仍能命中 |
| 6.8 | `-O0` vs `-O2` | `sizeof(jmp_buf)` = **200**；非 `volatile` 局部变量在 `-O2` 下回滚成 100，`-O0` 下是 200 |
