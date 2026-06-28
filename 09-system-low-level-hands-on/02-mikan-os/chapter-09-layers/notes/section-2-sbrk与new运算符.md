## 2. sbrk 与 new 运算符

> Ch4 用 **Placement new** 在静态数组构造对象 — 本章启用 **标准 `new`**。

---

### 一、Newlib 与堆

**Newlib** 的 **`malloc`/`free`** 依赖底层 **`sbrk()`**（或 `_sbrk`）扩展 **program break**：

```
malloc 需要更多内存 → 调用 sbrk(increment) → 获得新堆空间
```

| 传统 Unix | **MikanOS（本章）** |
|-----------|---------------------|
| 内核 `sbrk` 扩 **进程堆** | **`sbrk` 转发到 BitmapMemoryManager::Allocate** |

---

### 二、桥接 Ch8 位图分配器

```cpp
void* sbrk(intptr_t increment) {
    // 按 increment 换算页数
    void* p = memory_manager.Allocate(pages);
    // 维护 static 堆顶指针 heap_end
    return old_heap_end;
}
```

| 效果 | 说明 |
|------|------|
| **`malloc`** | 可用 — Console、图层对象内部缓冲 |
| **`new Window(...)`** | C++ **动态创建 Layer/Window** |
| **对齐 Ch8** | 所有堆内存仍是 **4KiB 页帧** 粒度（底层） |

→ [Ch8 BitmapMemoryManager](../chapter-08-memory/notes/section-6-位图管理器与首次适配.md) · [Ch5 Newlib](../chapter-05-console-text/notes/section-5-Console与Newlib.md)

---

### 三、注意点

| 项 | 说明 |
|----|------|
| **`delete` / `free`** | 需实现对应 **Free** — 书中随 Window/Layer 生命周期完善 |
| **碎片** | first fit + 频繁 new/delete — 教学阶段可接受 |
| **Ch27** | 应用级堆管理 **进一步细化** |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. LayerManager](./section-3-Window与LayerManager.md)
