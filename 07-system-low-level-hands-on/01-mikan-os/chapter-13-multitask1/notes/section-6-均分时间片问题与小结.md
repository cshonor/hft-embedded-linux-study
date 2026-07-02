## 6. 均分时间片问题与小结

---

### 一、性能回归：鼠标卡顿

**抢占 + Round-Robin 均分 20ms：**

| Task | 需求 | 实际 |
|------|------|------|
| **主任务（鼠标/绘制）** | **尽快** 响应输入 | 最多等 **(N-1)×20ms** |
| **Idle 等无用 Task** | 可 **不占** CPU | 仍分 **完整 20ms** |

**现象：** 鼠标移动 **明显迟滞、卡顿** — 交互 **劣于 Ch12 单循环**（讽刺但真实）。

**根因：** **调度策略** 未区分 **优先级/IO 密集 vs 空闲**。

---

### 二、Ch14 伏笔

| 方向 | 说明 |
|------|------|
| **任务休眠（sleep）** | 无事 Task **让出 CPU** — 不空占片 |
| **优先级调度** | 鼠标/输入 **更高优先级** — 更短有效等待 |
| **（可选）时间片差异化** | 主任务更长片或 **多级反馈** |

→ [chapter-14-multitask2](../chapter-14-multitask2/) 🔴

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **Context** | 寄存器+栈快照 |
| **SwitchContext** | 汇编 **合作式** 切换 |
| **20ms 抢占** | 定时器 **强制** 轮转 |
| **TaskManager** | **vector** 动态 Task |
| **债务** | **均分片** → 鼠标卡 |

```
Ch11 时钟
    ↓
Ch13 多任务引擎  ← 本章
    ↓
Ch14 调度策略优化
    ↓
Ch19–20 分页/syscall · 用户态应用
```

---

### 四、后续索引

| Ch13 主题 | 继续读 |
|----------|--------|
| 休眠/优先级 | [chapter-14-multitask2](../chapter-14-multitask2/) 🔴 |
| ULK 调度 | [chapter-07-process-scheduling](../../../04-Understanding-Linux-Kernel/chapter-07-process-scheduling.md) |
| 01 多任务 | [01 Day 15–16](../../02-30days-os/day-15-multitask1/) |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) |

---

← [5. TaskManager](./section-5-Task与TaskManager.md) · [Ch 12](../chapter-12-keyboard/) · [Ch 13 导读](../README.md)
