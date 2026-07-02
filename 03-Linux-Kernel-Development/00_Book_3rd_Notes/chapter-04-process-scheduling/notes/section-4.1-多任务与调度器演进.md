## ① 多任务与调度器演进

Linux = **抢占式多任务（Preemptive Multitasking）**：

| 概念 | 含义 |
|------|------|
| **调度器** | 决定 **哪个进程运行、何时、多久** |
| **抢占** | 强制停掉当前任务，换另一个上 CPU |

#### 历史脉络

```
简单调度器
    │
    ▼
O(1) 调度器（2.5）── 大服务器扩展性好；交互/低延迟场景欠佳
    │
    ▼
CFS（2.6.23+）────── 默认公平调度，兼顾交互响应
```

| 调度器 | 特点 |
|--------|------|
| **O(1)** | 每 CPU 固定时间片数组 — **O(1)** 选下一个；海量任务可扩展 |
| **CFS** | **完全公平** — 按权重分 CPU **比例**，非固定绝对时间片 |

→ [03 SysPerf §3.2 O(1)→CFS](../../../../13-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [§6.4 CFS/affinity](../../../../13-Systems-Performance-2nd/chapter-06-cpus/notes/section-6.4-硬件与软件架构.md)

---
