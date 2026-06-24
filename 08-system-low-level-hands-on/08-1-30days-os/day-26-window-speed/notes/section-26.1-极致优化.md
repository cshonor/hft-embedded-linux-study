## ① 极致优化 · 窗口与鼠标移动

#### 32 位并发写入

Day 4–11 刷新多 **逐字节** 写 VRAM / sheet buf。

| 观察 | 利用 |
|------|------|
| x86 **对齐 4 字节** 地址时 **`MOV [mem], EAX`** 与写 1 字节 **耗时相近** | 一次写 **4 像素/4 字节** |
| 理论 | 绘图 **~4×**（在可对齐区域） |

**改 `sheet_refreshmap` / `sheet_refreshsub`** — 内层循环 **32 位块写**。

**HFT：** **SIMD / 64B cache line / `memcpy` 宽写** — 同一 **「对齐 + 宽 store」** 原则。

→ [Day 10 局部 refresh](../day-10-layers/) · [01-CSAPP Ch6 对齐](../../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/)

#### 省略多余刷新 · new_mx / new_my

**快拖窗口/鼠标** → 连续 **FIFO 鼠标事件** → 若 **每事件全屏 refresh** → **卡**。

| 变量 | 作用 |
|------|------|
| **`new_mx`, `new_my`** | 暂存 **最新目标坐标** |
| **刷新时机** | **仅当 FIFO 空**（这一波移动事件 **处理完**）再 **`sheet_slide` + refresh** |

```
mousemove ×N → 只更新 new_mx/my
FIFO 空      → 一次 slide + refresh
```

**批处理输入事件** — 与 Day 23 **批量 refresh** 同族。

**效果：** 「唰唰唰」→ **「嗖嗖嗖」**。

---
