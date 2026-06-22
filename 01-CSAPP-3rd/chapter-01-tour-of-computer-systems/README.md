# Ch 1 计算机系统漫游 · A Tour of Computer Systems

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**

> 本章定位：**全书地图** — 用 `hello.c` 从源码到进程、内存、文件、网络走一遍，把后面 11 章要用的名词先亮出来。HFT 不必背细节，但要能 **指着这条链路说清延迟从哪来**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1.1 信息就是位 + 上下文 | [notes/section-1.1-信息就是位与上下文.md](./notes/section-1.1-信息就是位与上下文.md) |
| 1.2–1.3 程序翻译与编译系统 | [notes/section-1.2-1.3-程序翻译与编译系统.md](./notes/section-1.2-1.3-程序翻译与编译系统.md) |
| 1.4 处理器读取并解释指令（含 1.4.1–1.4.2） | [notes/section-1.4-处理器与hello程序生命周期.md](./notes/section-1.4-处理器与hello程序生命周期.md) |
| 1.5 高速缓存至关重要 | [notes/section-1.5-高速缓存至关重要.md](./notes/section-1.5-高速缓存至关重要.md) |
| 1.6 存储设备形成层次结构 | [notes/section-1.6-存储器层次结构.md](./notes/section-1.6-存储器层次结构.md) |
| 1.7 操作系统管理硬件（含 1.7.1–1.7.4） | [notes/section-1.7-操作系统管理硬件.md](./notes/section-1.7-操作系统管理硬件.md) |
| 1.8 系统之间利用网络通信 | [notes/section-1.8-网络通信.md](./notes/section-1.8-网络通信.md) |
| 1.9 重要主题（阿姆达尔 / 并发 / 抽象） | [notes/section-1.9-重要主题-Amdahl与并发与抽象.md](./notes/section-1.9-重要主题-Amdahl与并发与抽象.md) |

---

## 大白话 · 本章一条线

> **`hello.c` 怎么变成屏幕上的 `hello, world`？**

```
源码 hello.c
  → 编译/链接 → 可执行文件 a.out
  → shell 加载 → 进程在 CPU 上跑
  → 读内存/缓存/磁盘 → 经 OS  syscall 写终端或 socket
```

**你要带走的不是背图，而是三个 HFT 直觉：**

1. **慢的不在 CPU 算力，在等数据** — cache miss、磁盘、网络、syscall（→ 1.5–1.6，深入 [Ch 6](../chapter-06-存储器层次结构.md)）
2. **程序从不「直接摸硬件」** — OS 用进程/线程/虚拟内存/文件抽象隔开（→ 1.7，深入 Ch 8–9）
3. **优化要有靶心** — 阿姆达尔：先打最慢的那段（→ 1.9，配合 [Ch 5](../chapter-05-optimizing-performance/)、[02-SysPerf](../../02-Systems-Performance-2nd/chapter-02-methodologies/)）

---

## 本章 Checklist

- [ ] 能画出：编译器 / 汇编器 / 链接器各产出什么（`.s` `.o` `a.out`）
- [ ] 能简述：取指–译码–执行–访存–写回；PC、寄存器、主存的角色
- [ ] 能解释：为何 L1/L2/L3 对延迟敏感；局部性（时间/空间）一句话
- [ ] 能区分：进程 vs 线程；虚拟地址 vs 物理地址；文件即字节流
- [ ] 能写阿姆达尔公式，并说明「优化 10% 代码若只占 1% 时间则几乎无效」
- [ ] 能指出 HFT 热路径上：哪些步骤可旁路（DPDK）、哪些必须 syscall

---

## HFT 精读捷径

```
Ch 1 地图（本章，选读速通）
  → Ch 6 缓存/局部性（🔴 与 Hennessy Ch2 交叉）
  → Ch 8–9 进程与虚拟内存
  → Ch 10–11 I/O 与网络
  → Ch 12 并发
  → 02-SysPerf 观测与方法论
```

**时间紧：** 1.4–1.6 + 1.7.1–1.7.3 + 1.9 精读；1.2–1.3、1.8 扫一眼；1.10 当总结。

---

## 相关章节

- 下一章：[../chapter-02-representing-information/](../chapter-02-representing-information/)
- 缓存深入：[../chapter-06-存储器层次结构.md](../chapter-06-存储器层次结构.md)
- 虚拟内存：[../chapter-09-虚拟内存.md](../chapter-09-虚拟内存.md)
- 性能方法论：[02-SysPerf Ch 2](../../02-Systems-Performance-2nd/chapter-02-methodologies/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
