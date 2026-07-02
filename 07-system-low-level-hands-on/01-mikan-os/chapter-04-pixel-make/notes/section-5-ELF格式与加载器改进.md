## 5. ELF 格式与加载器改进

> **修复 Ch 3 加载器** — 内核内存大小计算错误 → 按 **LOAD 段** 精确加载。

---

### 一、ELF 文件结构（复习 + 加深）

```
┌──────────────────┐
│  ELF Header      │  魔数、架构、Entry Point、e_phoff…
├──────────────────┤
│  Program Headers │  加载视角 — Loader 主要读这里
├──────────────────┤
│  .text / .data…  │  各节内容
├──────────────────┤
│  Section Headers │  链接/调试视角 — 内核运行时 Loader 可不解析
└──────────────────┘
```

| 结构 | Loader 关心 |
|------|-------------|
| **ELF Header** | **`e_entry`** — 内核入口虚拟地址 |
| **Program Header** | 类型 **`PT_LOAD`** — 需装入内存的段 |
| **Section Header** | 可选 — 符号表等（后续调试） |

---

### 二、LOAD 段（PT_LOAD）

每个 **Program Header** 描述一段要映射的内容：

| 字段（示意） | 含义 |
|--------------|------|
| **p_vaddr** | 加载到内存的 **虚拟地址** |
| **p_memsz** | 内存中占用大小（含 BSS 零填充） |
| **p_filesz** | 文件中实际字节数 |
| **p_offset** | 在 ELF 文件中的偏移 |

```
p_filesz < p_memsz  →  .bss 段 — 文件后补零
多个 PT_LOAD        →  .text + .data + … 地址范围可能不连续
```

**Ch 3 错误（概念）：** 可能 **低估或误算** 内核占用的 **虚拟地址范围** → 拷贝越界或入口错。

---

### 三、改进后的加载流程

```
1. 读整个 kernel.elf 到临时缓冲（UEFI AllocatePool）
2. 校验 ELF Header（EM_X86_64 等）
3. 扫描所有 Program Header：
      对 PT_LOAD：
          更新 [min_vaddr, max_vaddr) 并集
4. 按并集大小 AllocatePages 分配物理页
5. 再次遍历 PT_LOAD：
      拷贝 p_filesz 字节到 (phys_base + (p_vaddr - min_vaddr))
      若 p_memsz > p_filesz → 剩余清零
6. 跳转到 phys_base + (e_entry - min_vaddr) 或按链接地址映射
7. 调用 KernelMain(…)
```

| 改进点 | 效果 |
|--------|------|
| **先算范围再分配** | 不浪费、不不足 |
| **按段拷贝** | **.text / .data / .bss** 各就各位 |
| **正确 e_entry** | RIP 指向真实 `_start` / `KernelMain` 链 |

→ [Ch1 PE/ELF/COFF](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md) · [Ch3 加载概览](../chapter-03-bootloader-display/notes/section-3-第一个内核与ELF加载.md)

---

### 四、与 Linux exec 的对照（概念）

| Mikan Loader | Linux 内核 |
|--------------|------------|
| 读 ELF PT_LOAD | `load_elf_binary` |
| 分配物理页 | `mmap` / 页表建立 |
| 跳 e_entry | 用户态 `_start` |

本章是 **极简子集** — 尚无完整页表（Ch 19）。

---

← [4. PixelWriter](./section-4-PixelWriter与vtable.md) · 下一节 [6. 小结](./section-6-小结与索引.md)
