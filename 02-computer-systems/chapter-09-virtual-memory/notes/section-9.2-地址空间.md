## 9.2 地址空间

> **Ch9 §9.2** · [章导读](../README.md) · 上节 [§9.1 ←](./section-9.1-物理和虚拟寻址·名字辨析.md) · 下节 [§9.3 →](./section-9.3-虚拟内存作为缓存工具.md)

---

- 每进程 **独立虚拟地址空间** — 同 VA 不同进程映射不同 PA  
- x86-64 用户空间典型 **48 位** 有效（256TB 量级概念）  
- **例：** 物理仅 8G；浏览器「以为」独占大 VA，实际可能只驻留 1G 物理页；多进程 VA 互不冲突；紧时页进 swap、VA 不变

---

### 常见陷阱
1. **48 位有效 VA 不等于 64 位** — x86-64 高 16 位是符号扩展（canonical address），非法地址触发 fault
2. **VA 空间大小 ≠ 物理内存大小** — 8GB 物理机器的进程仍可拥有 128TB VA 空间，靠按需分配+swap 实现
3. **同 VA 不同进程映射不同 PA** — 两个进程的 0x400000 完全独立，这是隔离的核心机制

### 自测题

<details>
<summary>Q1: x86-64 用户空间有效 VA 位数是多少？对应的地址空间大小？</summary>

48 位有效（高 16 位符号扩展），用户空间 0x0000000000000000–0x00007FFFFFFFFFFF，约 256TB。

</details>

<details>
<summary>Q2: 为什么物理内存只有 8GB，进程却可以「以为」拥有 256TB 地址空间？</summary>

VA 空间是逻辑概念，只有被访问到的页才分配物理页帧（demand paging），冷页可 swap 到磁盘。VA→PA 映射由页表维护。

</details>

<details>
<summary>Q3: 两个进程的虚拟地址 0x400000 指向同一物理地址吗？</summary>

不指向。每进程有独立页表，同 VA 映射不同 PA。除非显式共享（如共享库、mmap MAP_SHARED），否则互不可见。

</details>

<details>
<summary>Q4: canonical address 是什么？非法 canonical 地址会怎样？</summary>

x86-64 要求 VA 高 16 位是低 48 位最高位的符号扩展。不满足此格式的地址触发 #GP（通用保护异常）。

</details>

---

← [§9.1 ←](./section-9.1-物理和虚拟寻址·名字辨析.md) · [本章导读](../README.md) · [§9.3 →](./section-9.3-虚拟内存作为缓存工具.md)
