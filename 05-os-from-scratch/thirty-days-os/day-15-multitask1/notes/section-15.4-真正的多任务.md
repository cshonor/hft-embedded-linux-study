## ④ 真正的多任务 · 调度进 ISR

#### 之前：协作式味道

A、B **自己代码里** 判断超时 → **主动** 调切换 — 应用 **知道** 多任务存在。

#### 之后：抢占式雏形

把 **`mt_taskswitch`** 移入 **定时器中断处理**：

```
PIT IRQ → inthandler20 → … → mt_taskswitch() → far-JMP 下一任务
```

| 角色 | 变化 |
|------|------|
| **`HariMain` / `task_b_main`** | **不再写切换逻辑** |
| **OS** | **强制** 每 tick/超时 **夺 CPU** |

**里程碑：** **纸娃娃 OS** 从 **单线程顺序** → **内核调度多任务** — 现代 OS **scheduler 在 interrupt/timer 里** 的原型。

→ [Day 12 PIT](../day-12-timer1/) · [Day 6 ISR 要短](../day-06-split-compile-irq/) — 切换成本计入 **中断禁止时间**

---
