# TLPI · 章节大纲与 HFT 裁剪

> **Michael Kerrisk** · *The Linux Programming Interface*（2nd ed.）·《Linux/UNIX 系统编程手册》  
> **定位：** 用户态系统 API（先会用，再读 LKD 看实现）— 见 [README](./README.md)  
> 标签：🔴 必读 · 🟡 选读 · ⚪ 跳过  
> 锁定读序：Phase3 在 LKD 前 — [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md)  
> **章号审计：** [CHAPTER-MAP.md](./CHAPTER-MAP.md) — **仅 Ch1–5 与书对齐；Ch6 起目录号≠书内章号**

## Part I · 已对齐书内章号（Ch1–5）

| 书内章 | 主题 | 目录 | 标签 | 要点 |
|--------|------|------|------|------|
| 1 | History and Standards | `chapter-01-introduction` | 🟡 快读 | **POSIX vs Linux 扩展**；内核+GNU |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` | **🔴** | 用户/内核、syscall、进程、fd、inode |
| 3 | System Programming Concepts | **`chapter-03-system-programming-concepts`** | **🔴** | syscall vs 库函数、`errno`、功能测试宏、`tlpi_hdr` |
| 4 | File I/O: Universal I/O Model | **`chapter-04-file-io-universal`** | **🔴** | `open/read/write/close/lseek`；短读/部分写 |
| 5 | File I/O: Further Details | **`chapter-05-file-io-further`** | **🔴** | 三层结构、dup、pread、fcntl、原子、非阻塞 |

⚠️ 另有错号目录 `chapter-05-file-attributes`（内容≈书内 **Ch15**），见映射表。  
⚠️ 勿与 **APUE Ch3** 混用；TLPI 通用 I/O 在 **第 4 章**。

## 其后章节（Ch6–64）

**文件夹序号是自编课程序号，不能当书内章号。**  
读主题时：先查 [CHAPTER-MAP.md](./CHAPTER-MAP.md)，再打开对应目录。

### HFT 优先主题（按**书内章号**；括号为现仓库目录）

| 书内章 | 主题 | 现目录（错号，仅定位用） |
|--------|------|--------------------------|
| 20–22 | 信号 | `chapter-10`…`12` |
| 23 | 定时器 | `chapter-13-timers-sleep` |
| 29–30 | 线程 | `chapter-22`…`23` |
| 35 | 调度优先级 | （散落；见 map） |
| 49 | mmap | `chapter-15-memory-mapping` |
| 50 | VM / mlock | `chapter-45-virtual-memory` |
| 56–61 | Socket | `chapter-46`…`53` |
| 63 | poll/epoll/alt I/O | `chapter-41`/`42`/`43`/`54`/`58` |

## HFT 最短路径（书内章号）

```
Ch2 → Ch3 → Ch4 → Ch5 → Ch20–21 → Ch23 → Ch29–30 → Ch35 → Ch49 → Ch56–61 → Ch63
```

→ 实验代码放各章 `chapter-*/code/` · 网络纵深 → [11-UNP](../11-UNP-Vol1/)

<details>
<summary>旧脚手架备忘（目录号不可信）</summary>

| 旧目录约 | 主题 | 实为书内 |
|----------|------|----------|
| 10–12 | 信号 | 20–22 |
| 15 | mmap | 49 |
| 22–23 | 线程 | 29–30 |
| 41–42 | poll/epoll | 63 |
| 46–53 | Socket | 56–61 |

</details>
