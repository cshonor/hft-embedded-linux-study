## 3. 主存储器与内存映射

---

### 一、主存储器（RAM）的软件视图

从软件角度，**主存储器 = 连续排列的字节**：

```
物理地址:  0x00000000  0x00000001  0x00000002  …  0xFFFFFFFF…
           ┌────┬────┬────┬─────
           │ B0 │ B1 │ B2 │ …     每个地址 1 字节
           └────┴────┴────┴─────
```

| 概念 | 说明 |
|------|------|
| **物理地址** | CPU / 内存控制器访问 RAM 的 **编号** |
| **字节寻址** | 最小可寻址单位 = **1 字节**（8 bit） |
| **容量** | 由硬件安装条数决定 — OS 需 **探测** 实际布局 |

→ [CSAPP Ch6 存储器层次](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [02-Hennessy 内存](../../../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/)

---

### 二、为何需要「内存映射」

RAM 并非 **全部空闲** 给未来的 MikanOS：

| 占用者 | 示例 |
|--------|------|
| **UEFI 固件** | 代码、数据、ACPI 表 |
| **硬件保留** | MMIO、显卡 framebuffer、设备寄存器映射 |
| **已加载的 EFI 应用** | MikanLoader 自身 |
| **可用** | 后续内核、堆、栈可使用的 **常规 RAM** |

**内存映射（Memory Map）** = 一张 **一维「地图」**，标明每个 **物理地址段** 的：

- **起始地址、长度**
- **类型**（可用 / 保留 / ACPI / LoaderCode …）
- **属性**（可执行、可缓存等）

---

### 三、UEFI 内存类型（概念）

`GetMemoryMap()` 返回的每项描述一段连续物理区域，类型包括但不限于：

| 类型（示意） | 含义 |
|--------------|------|
| **EfiConventionalMemory** | **可用常规内存** — OS 分配器主要目标 |
| **EfiLoaderCode / EfiLoaderData** | 当前 UEFI 加载器使用 |
| **EfiACPIReclaimMemory** | ACPI 表等 |
| **EfiReservedMemoryType** | 保留，不可随意使用 |
| **MMIO 相关类型** | 设备映射，**不是普通 RAM** |

**OS 开发原则：** 只把 **ConventionalMemory**（及规范允许的类型）纳入 **自有物理页池** — 误用 MMIO/固件区会导致 **崩溃或硬件异常**。

---

### 四、与后续章节的关系

```
Ch 2  memmap CSV — 「物理世界真相」快照
    ↓
Ch 8  物理/线性内存管理 — 在可用区内分配
    ↓
Ch 19 分页 — 虚拟地址 ↔ 物理页
```

→ 对照 Linux **`/proc/iomem`**、x86 **e820 表**（固件传递的同类信息）

---

← [2. EDK II](./section-2-EDK-II与MikanLoader.md) · 下一节 [4. GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)
