# CSAPP 3rd — 全书目录（12 章 + 附录）

> **Computer Systems: A Programmer's Perspective** · Bryant & O'Neill · 第三版

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 · 默认不建笔记 |

---

## 第 1 章（独立）

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | A Tour of Computer Systems | [chapter-01](./chapter-01-tour-of-computer-systems/) | 🟡 |

## Part I · Program Structure and Execution（程序结构和执行）

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 2 | Representing and Manipulating Information | [chapter-02](./chapter-02-信息的表示和处理.md) | ⚪ |
| 3 | Machine-Level Representation of Programs | [chapter-03](./chapter-03-程序的机器级表示.md) | 🟡 |
| 4 | Processor Architecture | [chapter-04](./chapter-04-处理器体系结构.md) | 🔴 |
| 5 | Optimizing Program Performance | [chapter-05](./chapter-05-优化程序性能.md) | 🔴 |
| 6 | The Memory Hierarchy | [chapter-06](./chapter-06-存储器层次结构.md) | 🔴 |

## Part II · Running Programs on a System（在系统上运行程序）

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 7 | Linking | [chapter-07](./chapter-07-链接.md) | ⚪ |
| 8 | Exceptional Control Flow | [chapter-08](./chapter-08-异常控制流.md) | ⚪ |
| 9 | Virtual Memory | [chapter-09](./chapter-09-虚拟内存.md) | 🔴 |

## Part III · Interaction and Communication（程序间交互和通信）

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 10 | System-Level I/O | [chapter-10](./chapter-10-系统级IO.md) | 🟡 |
| 11 | Network Programming | [chapter-11](./chapter-11-网络编程.md) | 🔴 |
| 12 | Concurrent Programming | [chapter-12](./chapter-12-并发编程.md) | 🔴 |

## 附录

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | Error Handling | [appendix-A](./appendix-A-错误处理.md) | 🟡 |

---

## HFT 推荐阅读顺序

```
阶段 1（SysPerf 之前）
  Hennessy Ch2 → CSAPP Ch6
  → CSAPP Ch8–9–12（进程/VM/锁）
  → 选读 CSAPP Ch4、Ch1

阶段 2 起
  → 02-SysPerf → 05-LKD → 06-Gorman …

阶段 5 回来读 CSAPP Ch10–11（网络）
```

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
