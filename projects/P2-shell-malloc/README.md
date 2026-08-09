# P2 — Mini Shell + 自制 malloc/free

> C 写一个能跑管道的小 shell，再手写一套 malloc/free，把"程序=机器"从理论变肌肉记忆。

## 三个部分（各自可 `make` 的工程）

| Part | 做什么 | 指南 | 可运行工程 |
|------|--------|------|------------|
| **A** | mini shell：fork/exec/pipe/重定向/信号 | [Part-A-shell.md](./Part-A-shell.md) | [`part-a-shell/`](./part-a-shell/) → `make && ./myshell` |
| **B** | malloc/free：隐式链表 → 显式链表 → 分离适配 | [Part-B-malloc.md](./Part-B-malloc.md) | [`part-b-malloc/`](./part-b-malloc/) → `make test` |
| **C** | C 语言特性练手 | [Part-C-c-exercises.md](./Part-C-c-exercises.md) | [`part-c-exercises/`](./part-c-exercises/) → `make run` |

> 需要 Linux/WSL（fork/管道）。Windows 原生 MinGW 跑不了 Part A。

**建议顺序：A → C(练习1-3) → B → C(练习4-5)**

理由：A 先建立 fork/exec 的手感，C 的前三个练习直接在 shell 代码上改，B 是独立的内存分配器项目，C 的后两个练习跟 B 关联。

## 项目目标

两个子任务合一个 Project，因为它们都直击 C 语言最硬核的指针/内存模型：

1. **mini shell** — fork/exec/wait/pipe/重定向，理解进程模型
2. **malloc/free** — 显式空闲链表 + 分离适配，理解堆内存布局
3. **C 语言特性** — 函数指针/宏/union/offsetof/likely，为 P2.5 桥梁项目铺路

## 交付物

### Part A：mini shell（→ [详细指南](./Part-A-shell.md)）

- [ ] Phase 1：能跑 `ls`（fork + execvp + waitpid）
- [ ] Phase 2：内置命令 cd / exit / pwd
- [ ] Phase 3：I/O 重定向 `>` `<`（dup2）
- [ ] Phase 4：多级管道 `a | b | c`（pipe + fd 管理）
- [ ] Phase 5：后台 `&` + 信号 Ctrl-C（SIGINT/SIGCHLD）

### Part B：malloc/free（→ [详细指南](./Part-B-malloc.md)）

- [ ] Phase 1：隐式空闲链表 + 合并 + 分裂
- [ ] Phase 2：显式空闲链表（next/prev 指针）
- [ ] Phase 3：分离适配（按大小类分链表）
- [ ] Phase 4：压力测试 + 吞吐量测试
- [ ] `myrealloc` / `mycalloc` 实现

### Part C：C 语言特性练手（→ [详细指南](./Part-C-c-exercises.md)）

- [ ] 练习 1：函数指针命令分发表（shell 内置命令）
- [ ] 练习 2：调试分配器宏（`#` 字符串化 + `__LINE__`）
- [ ] 练习 3：union 多态值类型（shell 变量 int/float/string）
- [ ] 练习 4：offsetof 验证 malloc 头块对齐
- [ ] 练习 5：likely/unlikely 热路径标注（shell 解析器）

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`01` c-language](../../01-c-language/) | 指针、结构体对齐、函数指针、宏、GNU-C 扩展 |
| [`02` computer-systems](../../02-computer-systems/) | 进程（fork/exec/pipe）、信号、虚拟内存、堆布局 |

## 前置

[P1](../P1-cpu-simulator/)（理解硬件执行模型）。

## 学习目标

- 进程创建/替换/等待的完整生命周期
- 文件描述符表、管道、重定向如何共用 fd 机制
- 堆的内部结构：头块、对齐、合并、碎片
- 虚拟地址空间布局（text/data/bss/heap/stack）
- C 语言特性（函数指针/宏/union）在真实项目里怎么用

## 里程碑

| 里程碑 | 完成标志 |
|--------|----------|
| M1 | shell 能跑单命令 + 内建命令（Phase 1-2） |
| M2 | shell 支持管道和重定向（Phase 3-4） |
| M3 | malloc 隐式链表跑通基础测试（Phase 1） |
| M4 | malloc 升级分离适配 + 压力测试通过（Phase 2-4） |
| M5 | C 练习全部完成，能解释每个练了什么概念 |
| M6 |（可选）用自己写的 malloc 链接进 shell，验证可用 |

## 参考模块

- [01-c-language/01-Primer-K-and-R-C/ch08-unix-system-interface/](../../01-c-language/01-Primer-K-and-R-C/ch08-unix-system-interface/) — K&R ch8 文件描述符/read/write
- [01-c-language/02-Pointers-on-C/](../../01-c-language/02-Pointers-on-C/) — 指针/内存模型/ABI
- [02-computer-systems/chapter-08-exceptional-control-flow/](../../02-computer-systems/chapter-08-exceptional-control-flow/) — CSAPP ch8 进程/信号/管道
- [02-computer-systems/chapter-09-virtual-memory/](../../02-computer-systems/chapter-09-virtual-memory/) — CSAPP ch9 malloc/堆/虚拟内存
- [02-computer-systems/chapter-10-system-io/](../../02-computer-systems/chapter-10-system-io/) — CSAPP ch10 fd/dup2/重定向

## 环境

- WSL Ubuntu 24.04（gcc 13.3 + make）
- 编译 shell：`gcc -Wall -Wextra -g shell.c parser.c executor.c builtin.c -o shell`
- 编译 malloc：`gcc -Wall -Wextra -g mm.c memlib.c -o mmtest`
