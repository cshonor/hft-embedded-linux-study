## 6. 位图管理器与首次适配

---

### 一、BitmapMemoryManager

**物理内存以 4KiB **页帧（Page Frame）** 为单位管理：**

| 概念 | 说明 |
|------|------|
| **页帧号** | `phys_addr / 4096` |
| **位图** | 每帧 **1 bit** — **0 = 空闲，1 = 已用** |
| **类接口** | **`Allocate(pages)`** · **`Free(addr, pages)`** |

```cpp
class BitmapMemoryManager {
    uint8_t* bitmap_;
    size_t frame_count_;
public:
    void* Allocate(size_t num_pages);
    void Free(void* ptr, size_t num_pages);
};
```

**位图自身** 占用 RAM — 初始化时从 **ConventionalMemory** 预留，**不参与再分配**。

---

### 二、首次适配（First Fit）

**`Allocate(n)` 算法：**

```
从 bit 0 扫描位图：
  找到第一段连续 n 个 0 的区间
  将该 n 位全部置 1
  返回起始页帧对应的物理/identity 虚拟地址
失败 → nullptr
```

| 算法 | 特点 |
|------|------|
| **First fit** | 实现简单 · 速度可接受 · 可能产生 **外部碎片**（空闲页不连续） |
| **vs best/worst fit** | 本书选 **first fit** — 教学足够 |

**`Free`：** 对应页帧 bit **清 0** — 合并相邻空闲（可选优化，基础版可不合并）。

---

### 三、与前后章关系

| 章 | 关系 |
|----|------|
| **Ch4 Placement new** | 曾用 **静态缓冲** — 现可 **Allocate 页** 作堆雏形 |
| **Ch9 图层** | 可 **动态分配** 层缓冲 |
| **Ch19 分页** | **每进程独立页表** — 脱离纯 identity |
| **Ch27 应用内存** | 用户态 **malloc** 上层建筑 |

---

### 四、本章总结

```
Memory Map → 可用物理区
    ↓
迁移栈 / GDT / 页表
    ↓
lgdt + Identity 4-level paging (CR3)
    ↓
BitmapMemoryManager (4KiB, first fit)
    ↓
OS 内存独立 — 不再依赖 UEFI AllocatePages
```

---

### 五、后续索引

| Ch8 主题 | 继续读 |
|----------|--------|
| 图层 | [chapter-09-layers](../chapter-09-layers/) ⚪ |
| 进程分页 | [chapter-19-paging](../chapter-19-paging/) 🔴 |
| 应用堆 | [chapter-27-app-memory](../chapter-27-app-memory/) 🔴 |
| Ch7 中断 | [chapter-07-interrupt-fifo](../chapter-07-interrupt-fifo/) |

---

← [5. 分页](./section-5-四级分页与身份映射.md) · [Ch 7](../chapter-07-interrupt-fifo/) · [Ch 8 导读](../README.md)
