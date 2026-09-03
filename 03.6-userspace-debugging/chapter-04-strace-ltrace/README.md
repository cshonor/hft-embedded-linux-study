# Ch4 strace / ltrace：系统调用与库调用追踪

> 🔴 精读 · 不靠 gdb 也能看程序「全程在干什么」

gdb 擅长「停下来看现场」，但很多问题要**看程序运行全程的调用流**——它调了哪些系统调用、参数是什么、阻塞在哪、有没有多余调用。strace 追踪**系统调用**（syscall），ltrace 追踪**库函数调用**（library call）。本章覆盖这两个动态追踪工具，与 Ch3 的 core 尸检形成互补：core 看「崩溃瞬间」，strace 看「运行全程」。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 4.1 strace 入门（基本用法 / 输出格式 / 参数与 errno 解读） | `notes/01-strace-basics.md` |
| 4.2 strace 实战分析（-c 统计 / -f 子进程 / -p attach / 阻塞与多余 syscall 定位） | `notes/02-strace-practical-analysis.md` |
| 4.3 ltrace 库调用追踪（与 strace 对比 / malloc-free 追踪） | `notes/03-ltrace-library-calls.md` |

---

## HFT 关联

精读。strace 是「不碰代码、不改二进制」就能看清程序行为的工具：

- **定位阻塞**：行情/下单进程「卡住」时，strace 看它停在哪个 `recv`/`read`/`futex`；
- **发现多余 syscall**：热路径里本可避免的 `gettimeofday`/`read`/系统调用是延迟杀手；
- **审计网络调用**：追踪 socket 建连、`send`/`recv` 的时序，还原下单链路。
