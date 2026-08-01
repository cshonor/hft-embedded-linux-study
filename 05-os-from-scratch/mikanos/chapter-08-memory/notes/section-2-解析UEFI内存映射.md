## 2. 解析 UEFI 内存映射

---

### 一、数据来源

| 途径 | 说明 |
|------|------|
| **Ch2 MikanLoader** | `GetMemoryMap()` 导出 **memmap** CSV |
| **内核启动参数** | Loader 将 **Memory Map 缓冲** 传给 `KernelMain` |
| **ExitBootServices 前** | 必须在固件移交前 **保存 MapKey 与描述符** |

**内核首要任务：** 遍历 **EFI_MEMORY_DESCRIPTOR** 数组，按 **Type** 分类每段物理区间。

---

### 二、可用 vs 不可用

| 类型（复习 Ch2） | OS 能否当空闲 RAM |
|------------------|-------------------|
| **EfiConventionalMemory** | ✅ **主要分配池** |
| **EfiLoaderCode/Data** | ⚠️ 过渡后可能可回收 — 需按书步骤 |
| **EfiACPIReclaimMemory** | 视阶段 — 常保留或后期回收 |
| **MMIO / Reserved** | ❌ **绝不能** 当普通 RAM |

**算法（概念）：**

```
for each descriptor:
    if type == ConventionalMemory:
        mark [phys_start, phys_start+pages) as candidate pool
    else:
        exclude or reserve
```

---

### 三、与位图分配器衔接

**BitmapMemoryManager** 初始化时：

1. 确定 **可管理物理地址范围**（来自映射）
2. 排除 **内核自身、GDT、页表、栈、位图本身** 占用的页
3. 对其余 **4KiB 页帧** 建位图 — 初始 **0 = 空闲**

→ [Ch6 BAR0 MMIO](../chapter-06-mouse-pci/notes/section-5-BAR0与xHC初始化.md) — MMIO 区 **不得** 进入分配池

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 迁移](./section-3-迁移栈与关键数据结构.md)
