# Ch 9 §1 PKMap 地址空间管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 1. PKMap 地址空间管理

内核在 **页表顶部** 保留 **持久内核映射 (Persistent Kernel Mapping, PKMap)** 窗口：

```
    高地址
    FIXADDR_START   ─── 固定映射区（如 APIC）
    …
    PKMAP_BASE      ═══ PKMap 区（约 <32MiB，~1024 页槽位）
    …
    PAGE_OFFSET     ─── 常规内核线性映射
```

| 概念 | 说明 |
|------|------|
| **`PKMAP_BASE` ~ `FIXADDR_START`** | **临时** 把 **HIGHMEM 物理页** 映射进 **内核可访问 VA** |
| **池很小** | 同时 **~1024** 个高端页映射 — **必须短借短还** |

**设计约束：** PKMap 是 **稀缺槽位** — 占着不 **`kunmap`** 会 **饿死** 其他需要 kmap 的路径。

---
