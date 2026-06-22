# Ch 10 系统级 I/O · System-Level I/O

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**（Part III 前置）

> 本章定位：**Unix 一切皆文件** — fd、`open/read/write`、元数据、重定向、**Rio 处理短计数**。网络 socket 也是 fd（→ Ch11）；HFT 热路径最终走向 **非阻塞 + epoll / 旁路**，但 **短计数、EINTR、fd 生命周期** 仍必备。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 10.1–10.4 Unix I/O 与读写 | [notes/section-10.1-10.4-Unix-IO与读写.md](./notes/section-10.1-10.4-Unix-IO与读写.md) |
| 10.5 Rio 健壮读写 | [notes/section-10.5-Rio包.md](./notes/section-10.5-Rio包.md) |
| 10.6–10.9 元数据、目录与共享 | [notes/section-10.6-10.9-元数据目录与共享.md](./notes/section-10.6-10.9-元数据目录与共享.md) |
| 10.10–10.12 标准 I/O 与选型 | [notes/section-10.10-10.12-标准IO与选型.md](./notes/section-10.10-10.12-标准IO与选型.md) |

---

## 大白话 · 本章一条线

> **内核用「文件描述符」统一管文件、管道、socket；`read`/`write` 可能一次只搬一部分字节。**

```
open → fd
read/write（可能 short count、EINTR）
stat 元数据 · dup2 重定向
Rio：帮你读到 n 字节为止
```

**HFT 三件事：**

1. **永远处理 short read/write** — 协议解析、日志写入
2. **分清 Unix I/O vs stdio** — 热路径不用 `printf`/`fread` 混 fd
3. **理解 fd 表与共享** — `fork`/`dup`、重定向、多进程网关

---

## 本章 Checklist

- [ ] 说出 fd 0/1/2；`open` 标志 `O_RDONLY`/`O_RDWR`/`O_APPEND`
- [ ] 解释 **短计数** 为何发生；`rio_readn` 与裸 `read` 区别
- [ ] 会用 `stat`/`fstat` 读 `st_mode`、`st_size`
- [ ] 理解 **内核 fd 表 vs 打开文件表 vs v-node** 共享关系
- [ ] 会用 `dup2` 做 I/O 重定向
- [ ] 能对比 **Unix I/O / 标准 I/O / mmap** 选型（10.11）

---

## HFT 精读捷径

```
必读：10.4 短计数 · 10.5 Rio · 10.11 选型
配置/冷路径：10.6 stat · 10.9 重定向
热路径网络：Ch10 打底 → Ch11 epoll → 08-UNP / DPDK 旁路
10.7 目录 · 10.10 stdio — 扫读
```

---

## 相关章节

- 上一章：[../chapter-09-virtual-memory/](../chapter-09-virtual-memory/)
- 下一章：[../chapter-11-网络编程.md](../chapter-11-网络编程.md)
- mmap：[../chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md](../chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md)
- UNP 深化：[08-UNP-Vol1](../../08-UNP-Vol1/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
