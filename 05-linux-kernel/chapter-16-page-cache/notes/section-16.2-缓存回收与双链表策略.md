## ② 缓存回收与双链表策略

内存紧张时需 **驱逐缓存页** — 优先换出 **干净** 页。

Linux 使用修改版 **LRU** — **双链表**：

| 链表 | 含义 |
|------|------|
| **active（活跃）** | **热数据** — **不会被回收** |
| **inactive（非活跃）** | **可回收候选** |

| 规则 | 说明 |
|------|------|
| 页在 inactive 被 **再次访问** | 提升到 **active** |
| 仅 inactive 上 **干净页** 可被回收 | |

```
解决什么问题？
  一次性顺序读大文件 — 若传统 LRU，会冲掉真正热的缓存
  双链表：一次扫描的页留在 inactive，不挤 active 热页
```

→ **Ch 12** 物理页回收



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 页面回收的 LRU 双链表策略是什么？

<details><summary>答案</summary>

active_list（活跃页）和 inactive_list（不活跃页）。新页进 inactive 尾部。被再次访问时从 inactive 提升到 active。回收从 inactive 尾部开始。这比单链表 LRU 更好：防止「一次扫描污染」（如 find / 扫描大量文件后不立即驱逐活跃数据）。HFT 用 mlock 锁页不参与回收。

</details>

</details>
---
