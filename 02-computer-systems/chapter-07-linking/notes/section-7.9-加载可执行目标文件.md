## 7.9 加载可执行目标文件

> **Ch7 §7.9** · [章导读](../README.md) · 上节 [§7.8 ←](./section-7.8-可执行目标文件.md) · 下节 [§7.10 →](./section-7.10-动态链接共享库.md)

---

- `execve` 内核 **创建进程地址空间**，映射 **PT_LOAD** 段
- **`.bss`** 分配零页；**栈、堆** 随后增长
- 运行时常见布局（高→低）：**栈 ↓ · 堆 ↑ · `.data`/`.bss` · `.rodata` · `.text`**
- 分区与 HFT（禁热路径 malloc）→ [Ch3 · 五大内存分区](../../chapter-03-machine-level-programs/notes/section-补充-C程序五大内存分区.md)
- 细节 → [Ch 9 虚拟内存](../chapter-09-virtual-memory/)

---

### 口述巩固 · 自测

1. **加载后谁先有内容？** `.text`/`.data` 映射进来；`.bss` 清零；栈/堆后长  
2. **`.bss` 为何不占文件体积？** 只记大小，运行时填 0  

---

← [§7.8 ←](./section-7.8-可执行目标文件.md) · [本章导读](../README.md) · [§7.10 →](./section-7.10-动态链接共享库.md)
