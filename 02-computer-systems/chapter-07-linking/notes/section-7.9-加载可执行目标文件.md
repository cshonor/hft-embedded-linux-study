## 7.9 加载可执行目标文件

> **Ch7 §7.9** · [章导读](../README.md) · 上节 [§7.8 ←](./section-7.8-可执行目标文件.md) · 下节 [§7.10 →](./section-7.10-动态链接共享库.md)

---

- `execve` 内核 **创建进程地址空间**，映射 **PT_LOAD** 段
- **`.bss`** 分配零页；**栈、堆** 随后增长
- 运行时常见布局（高→低）：**栈 ↓ · 堆 ↑ · `.data`/`.bss` · `.rodata` · `.text`**
- 分区与 HFT（禁热路径 malloc）→ [Ch3 · 五大内存分区](../../chapter-03-machine-level-programs/notes/section-补充-C程序五大内存分区.md)
- 细节 → [Ch 9 虚拟内存](../../chapter-09-virtual-memory/)

---

### 口述巩固 · 自测

1. **加载后谁先有内容？** `.text`/`.data` 映射进来；`.bss` 清零；栈/堆后长  
2. **`.bss` 为何不占文件体积？** 只记大小，运行时填 0  

### 自测题

<details>
<summary>1. `execve()` 加载可执行文件时发生了什么？</summary>

1. OS 创建新进程的虚拟地址空间
2. 读 ELF 程序头表，把各 segment 映射到虚拟内存（`.text`→代码段 r-x，`.data`/`.bss`→数据段 rw-）
3. 设置栈（argc, argv, envp）
4. 跳到 `_start`（C 运行时入口）→ `__libc_start_main` → `main()`
5. `main` 返回后 `exit()` 调用 `.fini` + 清理

**关键**：入口不是 `main`，而是 `_start`——它设置好运行时环境后才调用 `main`。

</details>


---

← [§7.8 ←](./section-7.8-可执行目标文件.md) · [本章导读](../README.md) · [§7.10 →](./section-7.10-动态链接共享库.md)
