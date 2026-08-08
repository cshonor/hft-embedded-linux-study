# P2 — Mini Shell + 自制 malloc/free

> C 写一个能跑管道的小 shell，再手写一套 malloc/free，把"程序=机器"从理论变肌肉记忆。

## 项目目标

两个子任务合一个 Project，因为它们都直击 C 语言最硬核的指针/内存模型：

1. **mini shell** — fork/exec/wait/pipe/重定向，理解进程模型
2. **malloc/free** — 显式空闲链表 + 分离适配，理解堆内存布局

## 交付物

### Part A：mini shell

- [ ] 解析命令行（含引号、管道 `|`、重定向 `>` `<`、后台 `&`）
- [ ] `fork` + `execvp` 执行外部命令
- [ ] 多级管道（`a | b | c`）
- [ ] 内建命令：`cd`、`exit`、`pwd`
- [ ] 信号处理（Ctrl-C 不退出 shell，只中断前台子进程）

### Part B：malloc/free

- [ ] `mymalloc`/`myfree` 接口（与 libc 同签名）
- [ ] 隐式空闲链表 → 升级为显式空闲链表
- [ ] 分离适配（segregated free lists，按大小类）
- [ ] 合并相邻空闲块（coalesce）
- [ ] 用 `sbrk`/`mmap` 向 OS 申请内存
- [ ] 写压力测试：随机 alloc/free 序列，验证无碎片泄漏

### Part C：C 语言特性练手（轻量）

> 在 shell/malloc 代码上直接加，不单独建项目。练 01-c-language 书 02（指针）+ 书 04（GNU C）的技能点，为 [P2.5](../P2.5-c-toolkit/) 做铺垫。

- [ ] **函数指针命令分发表** — 替换 if-else 链，`struct { char *name; void (*fn)(char**); }` 数组 + 查找循环
- [ ] **调试分配器宏** — `#define MALLOC(sz) debug_malloc((sz), __FILE__, __LINE__)`，练 `#` 字符串化 + `__LINE__` 宏
- [ ] **`union` 多态值类型** — shell 变量存 int/float/string，`enum` 做 tag，`switch` 分发
- [ ] **`offsetof` 验证头块对齐** — `#include <stddef.h>`，打印 malloc 头块各成员偏移
- [ ] **`likely`/`unlikely` 热路径标注** — GNU C 扩展，包裹 shell 解析器高频分支

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`01` c-language](../../01-c-language/) | 指针、结构体对齐、GNU-C、ABI |
| [`02` computer-systems](../../02-computer-systems/) | 进程（fork/exec）、虚拟内存、堆布局、CSAPP Ch9 |

## 前置

[P1](../P1-cpu-simulator/)（理解硬件执行模型）。

## 学习目标

- 进程创建/替换/等待的完整生命周期
- 文件描述符表、管道、重定向如何共用 fd 机制
- 堆的内部结构：头块、对齐、合并、碎片
- 虚拟地址空间布局（text/data/bss/heap/stack）

## 里程碑

1. **M1** shell 能跑单命令 + 内建命令
2. **M2** shell 支持管道和重定向
3. **M3** malloc 隐式链表跑通基础测试
4. **M4** malloc 升级分离适配 + 合并，通过压力测试
5. **M5** 用自己写的 malloc 链接进 shell，验证可用

## 参考模块

- [01-c-language/](../../01-c-language/) — K&R、C 和指针、嵌入式自我修养（GNU C 扩展）
- [02-computer-systems/](../../02-computer-systems/) — CSAPP Ch8（异常控制流/进程）、Ch9（虚拟内存/mmap）

## 环境

- WSL Ubuntu 24.04（gcc 13.3 + make）—— 见项目长期记忆
- 编译：`gcc -Wall -Wextra -g shell.c -o shell`
