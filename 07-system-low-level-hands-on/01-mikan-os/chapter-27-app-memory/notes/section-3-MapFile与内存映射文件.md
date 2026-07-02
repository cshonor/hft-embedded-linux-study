## 3. MapFile 与内存映射文件

---

### 一、动机

**Ch25 read/lseek：** 顺序/随机读 — **每次 syscall 拷贝**。

**Memory-Mapped File：** 文件 **映到 VA** — 应用 **`*(char*)addr` 即读**。

---

### 二、MapFile() syscall

```cpp
void* SyscallMapFile(int fd, size_t offset, size_t len) {
    auto* file = CurrentTask()->GetFD(fd);
    void* va = ReserveMapping(len);
    RegisterFileMapping(va, len, file, offset);
    // PTE: Present=0 或 special — 等 #PF
    return va;
}
```

**首次访问某页：**

```
#PF @ va
HandlePageFault:
    frame = AllocateFrame()
    ReadFileClusterInto(frame, file, file_offset_for_va)
    MapPage(pml4, va, frame, User|Readonly)
```

| 点 | 说明 |
|----|------|
| **Page Cache** | 帧内 **文件切片** — 按需 **读盘/内存卷** |
| **与 Demand 结合** | **不预读全文件** — **按页 fault-in** |
| **随机访问** | **O(1) touch** 经 **页对齐** 映射 |

→ [08 TLPI mmap](../../../06-The-Linux-Programming-Interface/) · [CSAPP Ch9 mmap](../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---

### 三、与 Open/Read 对比

| | **read()** | **MapFile** |
|---|------------|-------------|
| 拷贝 | **内核→用户 buffer** | **映射后直读** |
| 大文件 | 需 **大 buffer/循环** | **按页加载** |
| 写 | Ch26 **Write** | 本书 **读映射为主** |

---

← [2. Demand Paging](./section-2-按需分页与DemandPages.md) · 下一节 [4. memstat](./section-4-memstat与位图统计.md)
