## ④ 任务层级 LEVEL · 优先级调度

平分时间片 → **拖窗口/移鼠标** 时 **B 计数仍占片** → **UI 卡**。

#### 优先级（Priority）

为任务设 **不同切换间隔**（如 **0.01s vs 0.02s**）→ **CPU 时间比例** 可调。

#### 层级（LEVEL）· 核心规则

**`TASKLEVEL`** — 任务分 **LEVEL 0 / 1 / 2 …**（数字越小通常 **越重要**，以原书为准）：

```
调度铁律：
  永远只运行 **当前最高 LEVEL** 里、且 **未 sleep** 的任务
  仅当该 LEVEL **全部 sleep** → 才降级看下一 LEVEL
```

| 配置（原书） | 效果 |
|--------------|------|
| **任务 A（UI/输入）** | **LEVEL 1** — 较高 |
| **B0～B2（纯计数）** | **LEVEL 2** — 较低 |

#### 交互场景

| 用户状态 | 系统行为 |
|----------|----------|
| **不操作** | A **sleep** → 只有 **LEVEL 2 的 B** 全速跑 |
| **碰鼠标/键** | FIFO **wake A** → A 在 **更高 LEVEL** → **立刻抢占 CPU** |
| 拖窗/输入 | **丝滑** — B **不会和 A 抢同等级时间片** |

```
         LEVEL 1:  [ A — UI ]     ← 有 wake 则独占这一层
         LEVEL 2:  [ B0 B1 B2 ]   ← 仅当 L1 全睡才跑
```

**里程碑：** **抢占式 + 优先级 + sleep/wake** — 「纸娃娃」有了 **现代调度器雏形**。

→ [03-Linux-Kernel-Development 调度/CFS 概念](../../../../03-Linux-Kernel-Development/) · [03 SysPerf 上下文切换](../../../../13-Systems-Performance-2nd/chapter-03-operating-systems/)

---
