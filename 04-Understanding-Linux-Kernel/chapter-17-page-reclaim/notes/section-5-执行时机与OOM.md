## 5. PFRA 执行时机与 OOM Killer

---

### 一、三种触发场景

| 场景 | 触发者 | 说明 |
|------|--------|------|
| **内存紧缺回收** | 分配失败路径 | `try_to_free_pages()` 等 |
| **周期性回收** | **`kswapd`** 内核线程 | 空闲页低于 **水位** 时后台扫描 |
| **休眠回收** | 挂起到磁盘 | hibernation 路径 |

---

### 二、内存紧缺：`try_to_free_pages()`

伙伴系统 / 缓存分配 **失败** 时：

```
try_to_free_pages()
    ↓ 至多 ~13 轮
shrink_caches()   — 页缓存、映射页等
shrink_slab()     — Slab 可回收对象
    ↓ 仍失败
out_of_memory() → OOM Killer
```

→ Slab：[Ch 8 section-3](../chapter-08-memory-management/notes/section-3-Slab分配器.md)

---

### 三、`kswapd` 与水位

每个 **内存节点** 有专属 **`kswapd`**：

- 监控 **空闲页框** vs **min/low/high 水位**  
- 低于阈值 → **唤醒** kswapd **后台** 回收 — **避免** 突然耗尽  

与 **pdflush** 写回脏页协同 — [Ch 15](../chapter-15-page-cache/notes/section-5-回写脏页与pdflush.md)。

---

### 四、OOM Killer（最后手段）

`out_of_memory()` 仍无法腾出足够页 → **选杀一进程**：

- 综合 **内存占用**、**子进程**、**root 加成**、**用户可调 oom_score_adj** 等  
- 发送信号终止 — 释放其 **全部用户页**  

> **深潜可选：** `select_bad_process()` / `badness()` — 2.6 `mm/oom.c`。

HFT：**关键进程** 调低 oom_score_adj 或 **cgroup memory** 隔离；交易进程 **`mlock`** 关键页。

---

← [4. LRU](./section-4-LRU链表.md) · 下一节 [6. Swap](./section-6-交换机制.md)
