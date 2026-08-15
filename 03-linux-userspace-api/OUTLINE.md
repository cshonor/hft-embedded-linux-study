# TLPI · 章节大纲与 HFT 裁剪

> **Michael Kerrisk** · *The Linux Programming Interface*（2nd ed.）  
> **定位：** 用户态系统 API — [README](./README.md)  
> 标签：🔴 必读 · 🟡 选读 · ⚪ 跳过  
> 锁定读序：Phase3 — [README](../README.md)  
> **一书一目录（Ch1–64）** — [CHAPTER-MAP.md](./CHAPTER-MAP.md)

## Part I · 基础（Ch1–5）

| 书内章 | 主题 | 目录 | 标签 |
|--------|------|------|------|
| 1 | History and Standards | `chapter-01-introduction` | 🟡 |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` | **🔴** |
| 3 | System Programming Concepts | `chapter-03-system-programming-concepts` | **🔴** |
| 4 | Universal I/O Model | `chapter-04-file-io-universal` | **🔴** |
| 5 | File I/O Further Details | `chapter-05-file-io-further` | **🔴** |

## HFT 优先

| 书内章 | 主题 | 目录 | 标签 |
|--------|------|------|------|
| 20–22 | 信号 | `chapter-20`…`21` | **🔴** |
| 23 | 定时器 | `chapter-23-timers-sleeping` | **🔴** |
| 29–30 | 线程 | `chapter-29`…`30` | **🔴** |
| 35 | 优先级 / 调度 | `chapter-35-process-priorities-scheduling` | **🔴** |
| 49 | mmap | `chapter-49-memory-mappings` | **🔴** |
| 50 | VM / mlock | `chapter-50-virtual-memory` | 🟡 |
| 56–61 | Socket | `chapter-56`…`61` | **🔴**/🟡 |
| **63** | **epoll 等** | `chapter-63-alternative-io` | **🔴** |

## HFT 最短路径

```
Ch2 → Ch3 → Ch4 → Ch5 → Ch20–21 → Ch23 → Ch29–30 → Ch35 → Ch49 → Ch56–61 → Ch63
```

→ 实验放各章 `code/` · 网络纵深 → [12-UNP](../03.5-unix-network-api/)
