## 9.9.1–9.9.6 malloc 基础与碎片

### 9.9.1 malloc 和 free

```c
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t size);
```

- 堆在 **program break** 之上增长 — `sbrk`/`brk`（内核）

### 9.9.2 为何动态分配

- 运行时才知道大小；复杂数据结构生命周期不一

### 9.9.3 分配器目标

| 目标 | 说明 |
|------|------|
| **吞吐** | malloc/free 快 |
| **空间利用率** | 少碎片 |
| **快速** | 常数或接近常数时间 |

### 9.9.4 碎片

- **内部碎片** — 分配块 > 请求
- **外部碎片** — 空闲内存总和够，但不连续

### 9.9.5–9.9.6 实现问题与隐式空闲链表

- 堆 = **连续字节数组** + **块头/脚** 元数据
- **隐式链表** — 遍历所有块（含已分配），找空闲 — 简单但慢

**HFT：**

- 生产用 **jemalloc / tcmalloc / mimalloc** 或 **自研 arena**
- **热路径** — 对象池、slab、**永不 free** 到通用堆（tick 内）

---

← [本章导读](../README.md)
