## 4. 性能测量与批量重绘

---

### 一、GetCurrentTick 系统调用

**客观测耗时：**

```cpp
uint64_t t0 = GetCurrentTick();
// … 绘图 …
uint64_t t1 = GetCurrentTick();
printf("elapsed: %lu ticks\n", t1 - t0);
```

| 来源 | Ch11 **APIC 定时器** 累计 tick |
|------|-------------------------------|
| 用途 | **优化前后对比** — 科学而非体感 |

→ [Ch11 定时器](../chapter-11-timer-acpi/)

---

### 二、瓶颈：逐点重绘

**早期 stars：** 每 **WinFillRectangle** 都 **SendMessage(kLayer Redraw)**。

```
N 颗星 × 全屏 Layer 合成 = O(N × 像素) — 极慢
```

**测量结论：** 数千点可 **卡数秒** — 不可用。

---

### 三、LAYER_NO_REDRAW 技巧

**复用 64 位 layer_id 参数的高 32 位：**

```cpp
constexpr uint64_t LAYER_NO_REDRAW = 0x00000000'00000000ULL; // 高32位置标志
// 实际: layer_id | (1ULL << 32) 等编码 — 按书实现

WinFillRectangle(lid | NO_REDRAW_FLAG, x, y, 1, 1, color);
// 内核: 只写 shadow buffer，不立即合成到屏幕
```

**全部画完后：**

```cpp
WinRedraw(lid);   // 一次性 kLayer 消息 · Main 合成一次
```

| 策略 | 效果 |
|------|------|
| **defer redraw** | CPU 不做 **N 次 BitBlt/合成** |
| **单次 WinRedraw** | **~99× 加速**（书中实测量级） |

**设计启示：** GUI **脏矩形/批量 flush** 是 **桌面 OS 基本优化** — 与 **游戏 double-buffer present** 同族。

---

### 四、WinRedraw syscall

**显式 **「现在显示 shadow」** — 与 **NO_REDRAW 标志** 配对。

→ [Ch9 Layer 消息](../chapter-09-layers/) · [Ch15 kLayer Redraw](../chapter-15-terminal/)

---

← [3. DoWinFunc](./section-3-WinFillRectangle与DoWinFunc.md) · 下一节 [5. DrawLine](./section-5-WinDrawLine与lines命令.md)
