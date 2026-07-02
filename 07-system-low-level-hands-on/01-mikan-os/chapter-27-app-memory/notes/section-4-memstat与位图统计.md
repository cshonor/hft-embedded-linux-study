## 4. memstat 与位图统计

---

### 一、位图分配器统计

**Ch8 BitmapPageAllocator：**

```cpp
struct MemoryStat {
    size_t total_frames;
    size_t allocated_frames;
    size_t free_frames;
};

MemoryStat GetMemoryStat();
```

| 计数 | 用途 |
|------|------|
| **total** | **物理内存池** 总 4KiB 帧 |
| **allocated** | **内核+应用+页表+缓存** 已占 |
| **free** | **剩余可分配** |

→ [Ch8 位图分配](../chapter-08-memory/notes/section-6-位图分配器与小结.md)

---

### 二、memstat 命令

```
> memstat
total: 32768 frames (128 MiB)
used:  1024 frames
free:  31744 frames
```

**类似 Linux `free`：** 开发 **Demand/CoW** 时 **直观看到节省**。

| 场景 | 观察 |
|------|------|
| **开 3 个 cube（CoW 前）** | used **线性涨** |
| **CoW 后** | **.text 共享** — used **涨幅小** |
| **malloc 大数组未 touch** | Demand — used **几乎不变** |

---

### 三、调试价值

**与 Ch27 特性联动验证：**

- **DemandPages** — 申请 **不 touch** → **memstat 不变**
- **MapFile** — 映射 **不读** → 同上
- **CoW** — 多开 **只读跑** → **共享帧**

---

← [3. MapFile](./section-3-MapFile与内存映射文件.md) · 下一节 [5. CoW](./section-5-写入时复制与invlpg.md)
