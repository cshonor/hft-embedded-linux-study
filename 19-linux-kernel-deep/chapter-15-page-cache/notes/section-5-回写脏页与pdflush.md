## 5. 回写脏页到磁盘 (Writing Dirty Pages)

> **延迟写** — 合并多次修改，提高响应速度

---

### 一、脏页 (Dirty Pages)

进程 **写** 页缓存 → 页标为 **dirty**：

- **不立即** 写磁盘  
- 多次写同一页 → **一次** 物理更新  
- 系统响应更快  

脏页统计影响 **内存回收** 决策（→ [Ch 17](../chapter-17-page-reclaim.md)）。

---

### 二、`pdflush` 内核线程池

ULK 2.6 用 **`pdflush`** — **2～8 个** 动态内核线程，后台 **刷新脏页**：

| 唤醒条件 | 说明 |
|----------|------|
| **脏页比例超阈值** | 通常 > 总页数 **~10%**（背景写回） |
| **脏页停留过久** | **周期性** 超时 — 防止脏页长期占内存 |

线程从 `address_space` / 全局脏页列表取页 → `writepage` → **bio** → 块层。

→ 块 I/O：[Ch 14](../chapter-14-block-devices/)

> **Modern 对照：** `pdflush` 已由 **per-backing-device `flush` 线程**（bdi_writeback）取代；机制仍为 **阈值 + 周期写回**。

> **深潜可选：** `pdflush_operation`、`background_writeout` — 2.6 `mm/page-writeback.c`。

---

### 三、与基数树标签配合

写回前用 **`PAGECACHE_TAG_DIRTY`** 快速 **定位脏页**，跳过干净子树 — [section-3](./section-3-基数树与标签.md)。

---

← [4. 缓冲页](./section-4-缓冲页与buffer_head.md) · 下一节 [6. sync syscall](./section-6-同步系统调用.md)
