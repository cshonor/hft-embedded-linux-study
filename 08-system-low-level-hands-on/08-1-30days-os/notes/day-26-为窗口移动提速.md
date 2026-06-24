## Day 26 · 为窗口移动提速

> **原书第二十六章** · **GUI 性能 + Shell 增强** — **32 位写 VRAM**、**new_mx/new_my 延迟刷新**、**Shift+F2** 多 Console、**`start` / `ncst`**。  
> ← [Day 25](./day-25-增加命令行窗口.md) · [08-1 导读](../README.md) · → Day 27（待补）

---

### 本节三段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 移动提速** | **32 位写** · **FIFO 空才刷** | 窗/鼠 **嗖嗖** |
| **② Console 管理** | 开机 **1 窗** · **Shift+F2** · **× 关 Console** |
| **③ start/ncst** | 新窗跑 app · **无黑框** 启动 | 孤立 app **安全退出** |

---

### ① 极致优化 · 窗口与鼠标移动

#### 32 位并发写入

Day 4–11 刷新多 **逐字节** 写 VRAM / sheet buf。

| 观察 | 利用 |
|------|------|
| x86 **对齐 4 字节** 地址时 **`MOV [mem], EAX`** 与写 1 字节 **耗时相近** | 一次写 **4 像素/4 字节** |
| 理论 | 绘图 **~4×**（在可对齐区域） |

**改 `sheet_refreshmap` / `sheet_refreshsub`** — 内层循环 **32 位块写**。

**HFT：** **SIMD / 64B cache line / `memcpy` 宽写** — 同一 **「对齐 + 宽 store」** 原则。

→ [Day 10 局部 refresh](./day-10-叠加处理.md) · [01-CSAPP Ch6 对齐](../../01-CSAPP-3rd/chapter-06-memory-hierarchy/)

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

### ② 完善命令行窗口 · 开与关

#### 启动只留一窗

Day 25 **多 `cons[]`** — 开机 **一堆黑框** 不雅 → **init 只开 1 个 Console**。

#### Shift+F2 · 动态新开

| 快捷键 | 行为 |
|--------|------|
| **Shift+F2** | **新建** 命令行窗口 |

#### × 真正可关 Console

修补逻辑 — **点 Console 标题栏 ×** → **关闭该 Console**（此前可能 **关不掉** 或行为错）。

→ [Day 24 × 关 app](./day-24-窗口操作.md) · [Day 25 cons[]](./day-25-增加命令行窗口.md)

---

### ③ start 与 ncst · 启动命令

#### `start`（仿 Windows）

**需求：** **保留当前 Console**，**另开环境** 跑程序。

```
start color2
    → 自动 new Console
    → 在新窗执行 color2.hrb
```

**一边调试/一边对比** — 多 **Shell 会话**。

#### `ncst`（no console start）

**纯 GUI app**（如 **color 测试窗**）— 背后 **黑 Console 碍眼**。

| 命令 | 行为 |
|------|------|
| **`ncst xxx`** | **分配 task、跑 xxx.hrb** · **不创建可见 Console** |

**配套修改：**

- API **无 Console 时不往虚空 putchar**
- app **结束 → 自动 task 终止** — 防 **孤儿任务**

**类似 Windows `start /B` 或 GUI-only 进程** — 桌面更干净。

→ [Day 20 cmd_app](./day-20-API.md) · [Day 25 TASK.cons](./day-25-增加命令行窗口.md)

---

### Day 26 小结

| 问题 | 答案 |
|------|------|
| 绘图怎么 4×？ | **4 字节对齐处 32 位写** |
| 拖窗为何还卡？ | 每事件 refresh → **FIFO 空才刷** |
| 开机几个 Console？ | **默认 1** · **Shift+F2** 加窗 |
| 怎么关 Console？ | **× 可关** |
| `start`？ | **新 Console + 跑指定 app** |
| `ncst`？ | **无 Console 后台跑 GUI app** |

---

### 检查单

- [ ] 解释 **32 位写** 与对齐前提
- [ ] 描述 **new_mx/my + FIFO 空刷新**
- [ ] 区分 **`start` vs `ncst`**
- [ ] 说清 **ncst 为何改 API 路由**
- [ ] 串 Day 10→23→26 **refresh 优化链**

---

← [Day 25](./day-25-增加命令行窗口.md) · [08-1 导读](../README.md) · Day 27（待补）
