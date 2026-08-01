## 2. Ring 3 与页表 User 位

---

### 一、Protection Ring

| 环 | 谁 | 权限 |
|----|-----|------|
| **Ring 0** | **内核** | 全硬件 · 所有内存（映射允许时） |
| **Ring 3** | **应用** | **受限** — 不能 **随意 I/O / MSR / 改 CR3** |

**x86-64 长模式：** 主要用 **CPL（当前特权级）** + **分页 U/S 位** — 段 **DPL** 仍通过 **GDT** 配置。

---

### 二、GDT 与 DPL

```cpp
// 内核代码/数据: DPL=0
// 用户代码/数据: DPL=3
SetCodeSegment(selector, 0);
SetDataSegment(selector, 0);
SetCodeSegment(user_cs, 3);
SetDataSegment(user_ds, 3);
```

| 切换 | 时机 |
|------|------|
| **CPL=3 运行应用** | `sysret` / **iretq** 返回用户 |
| **CPL=0 内核** | 中断 · **syscall** 入口 |

→ [Ch8 GDT](../chapter-08-memory/notes/section-4-GDT与分段.md)

---

### 三、页表 User 标志

**Ch19 应用页：** 多为 **Supervisor only**。

**Ch20 调整：**

| 区域 | 页表项 |
|------|--------|
| **应用 .text/.data/.stack** | **Present \| Writable \| User** |
| **内核映射** | **无 User 位** — Ring3 **访问 → #PF** |

**效果：** 即使用 **错误指针** 也 **读不到内核物理页**。

→ [Ch19 U/S 铺垫](../chapter-19-paging/notes/section-3-虚拟地址与地址转换.md)

---

### 四、与 Ch18 执行模型对比

| Ch18–19 | Ch20 |
|---------|------|
| 内核线程 **call 应用** | 独立 **用户栈** · **CPL=3** |
| 应用 **ret 回终端** | **syscall / sysret** 或异常返回 |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. TSS](./section-3-TSS与RSP0内核栈.md)
