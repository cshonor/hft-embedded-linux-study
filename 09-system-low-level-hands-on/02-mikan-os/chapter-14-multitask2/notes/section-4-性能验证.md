## 4. 性能验证

---

### 一、实验设计

| 配置 | 行为 |
|------|------|
| **Ch13** | Main + **TaskB**（计数）均 **20ms 轮转** — Main 空转也占片 |
| **Ch14** | Main **无消息 Sleep** — CPU 多给 TaskB |

---

### 二、测量结果（书中）

| 指标 | 结果 |
|------|------|
| **TaskB 计数速度** | Main 休眠后 **约 1.4×** 提升 |
| **含义** | 原先 Main **浪费的片** 转移给 **后台 Task** |

**方法论：** 与 [Ch9 APIC 计时](../chapter-09-layers/notes/section-4-Local-APIC定时器测量.md) · [Ch11 tick](../chapter-11-timer-acpi/) 一致 — **先量化**。

---

### 三、尚未解决的部分

Sleep  alone **不能** 解决 **Main 有事件仍要等 TaskB 跑完 20ms** — 需 **§5 优先级** 才彻底消 **鼠标卡**。

---

← [3. 消息队列](./section-3-每任务消息队列与事件驱动.md) · 下一节 [5. 优先级](./section-5-任务优先级Level.md)
