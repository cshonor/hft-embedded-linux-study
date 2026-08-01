## 2. 按需分页与 DemandPages

---

### 一、Demand Paging 概念

| Eager（Ch19） | Demand（Ch27） |
|---------------|----------------|
| 加载 ELF **立即 AllocatePage 填 PTE** | 仅建 **PTE Present=0** 或 **映射占位** |
| 启动慢 · 占 RAM | **首次 touch 才 #PF 分配** |

```
app: mov [heap_va], rax
CPU: #PF — Present=0
OS:  HandlePageFault → alloc frame → PTE Present|RW|User
     retry instruction
```

---

### 二、HandlePageFault()

```cpp
void HandlePageFault(uint64_t cr2, PageFaultError code) {
    if (!IsUserFault(code)) { KernelPanic(); return; }
    if (IsDemandRegion(cr2)) {
        auto frame = AllocateFrame();
        ZeroPage(frame);                    // 匿名页
        MapPage(CurrentTask()->pml4, cr2, frame, User|Writable);
        return;
    }
    if (IsCoWRegion(cr2)) { HandleCoW(cr2); return; }
    if (IsMappedFile(cr2)) { FillFromPageCache(cr2); return; }
    KillApp(...);   // 真非法访问
}
```

| 路径 | 动作 |
|------|------|
| **匿名堆/栈扩展区** | **零页** |
| **MapFile 区** | 见 §3 |
| **CoW** | 见 §5 |

→ [Ch24 CPL=3 PF](../chapter-24-multi-terminal/notes/section-5-用户态异常与KillApp.md)

---

### 三、DemandPages() 与 sbrk

**syscall：** 为 **连续 VA 范围** 登记 **demand 区域**（**不立即占帧**）。

```cpp
void* SyscallDemandPages(size_t bytes) {
    return CurrentTask()->ExpandHeapDemand(bytes);
}
```

**newlib sbrk 重写：**

```c
void* sbrk(intptr_t inc) {
    return SyscallInvoke(SYS_DEMAND_PAGES, inc);
}
```

| 效果 | 说明 |
|------|------|
| **malloc 大块** | **VA 增长** · **物理帧按需** |
| **省 RAM** | 未 touch 的页 **不占帧** |

→ [Ch25 sbrk 初版](../chapter-25-app-read-file/notes/section-5-OpenFile-ReadFile与Newlib.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. MapFile](./section-3-MapFile与内存映射文件.md)
