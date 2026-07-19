## 9.9.1 malloc 和 free

> **Ch9 §9.9.1** · [章导读](../README.md) · 上节 [§9.8 ←](./section-9.8-内存映射mmap.md) · 下节 [§9.9.2 →](./section-9.9.2-为何动态分配.md)

---

```c
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t size);
```

- 堆在 **program break** 之上增长 — `sbrk`/`brk`（内核）

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§9.8 ←](./section-9.8-内存映射mmap.md) · [本章导读](../README.md) · [§9.9.2 →](./section-9.9.2-为何动态分配.md)
