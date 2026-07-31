# TLPI · 章节大纲与 HFT 裁剪

> **Michael Kerrisk** · *The Linux Programming Interface*（2nd ed.）  
> **定位：** 用户态系统 API — [README](./README.md)  
> 标签：🔴 必读 · 🟡 选读 · ⚪ 跳过  
> 锁定读序：Phase3 — [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md)  
> **目录号 = 书内章号** — 全表见 [CHAPTER-MAP.md](./CHAPTER-MAP.md)

## Part I · 基础（Ch1–5）

| 书内章 | 主题 | 目录 | 标签 |
|--------|------|------|------|
| 1 | History and Standards | `chapter-01-introduction` | 🟡 |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` | **🔴** |
| 3 | System Programming Concepts | `chapter-03-system-programming-concepts` | **🔴** |
| 4 | Universal I/O Model | `chapter-04-file-io-universal` | **🔴** |
| 5 | File I/O Further Details | `chapter-05-file-io-further` | **🔴** |

## HFT 优先（书内章 = 目录号）

| 书内章 | 主题 | 目录 | 标签 |
|--------|------|------|------|
| 20–22 | 信号 | `chapter-20`…`22` | **🔴** |
| 23 | 定时器 | `chapter-23-timers-sleeping` | **🔴** |
| 29–30 | 线程 | `chapter-29`…`30` | **🔴** |
| 49 | mmap | `chapter-49-memory-mappings` | **🔴** |
| 50 | VM / mlock | `chapter-50-virtual-memory` | 🟡 |
| 56–61 | Socket | `chapter-56`…`61` | **🔴**/🟡 |
| **63** | **epoll 等** | `chapter-63-epoll` 等 | **🔴** |

## HFT 最短路径

```
Ch2 → Ch3 → Ch4 → Ch5 → Ch20–21 → Ch23 → Ch29–30 → Ch49 → Ch56–61 → Ch63
```

→ 实验放各章 `code/` · 网络纵深 → [11-UNP](../11-UNP-Vol1/)

尚缺脚手架的书内章（8/10/11/13/25/26/28/32/35/42/45）见 CHAPTER-MAP；读时直接对纸书。
