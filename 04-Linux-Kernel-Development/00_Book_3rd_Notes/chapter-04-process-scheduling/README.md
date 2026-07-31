# Ch 4 进程调度 · Process Scheduling

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：**谁跑、何时跑、跑多久** — 抢占、**CFS**、休眠/唤醒、内核抢占、**RT**、affinity。  
> 前置：[Ch3 §3.8 PID/FD](../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)

---

## 推荐阅读顺序（原「连贯导读」已并入本章）

```
4.1  抢占式多任务 · runqueue · O(1)→CFS
  → 4.2  谁优先/跑多久 · nice vs RT · I/O vs CPU
  → 4.3  CFS 深挖（vruntime / 红黑树）  与/或  4.6 FIFO/RR
  → 4.5 / 4.7  抢占切换 · syscall/affinity
```

| 一页记忆 | |
|----------|--|
| 死循环卡不死整机？ | **抢占**（4.1） |
| 网络服务默认还行？ | CFS **vruntime** 偏爱常休眠（4.2/4.3） |
| CFS 怎么公平？ | 权重瓜分 CPU；**`vruntime` 最小**（红黑树最左）先跑 |
| 初始 weight？ | 由 **nice** 定；默认 0→1024；`fork` 继承，`exec` 不改 |
| nice ≠ 实时 | RT 另套；nice **调不动** RT（4.2/4.6） |
| 六策略谁最高？ | **DEADLINE > FIFO/RR > NORMAL > BATCH > IDLE** |
| HFT 热路径 | 隔离核 + FIFO；控制面留 CFS |

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 演进** | 抢占式多任务 | O(1) → **CFS（2.6.23）** |
| **② 策略** | I/O vs CPU · 六策略 · 优先级 | **nice** · **RT** · DEADLINE |
| **③ CFS** | 公平调度算法 | **`vruntime`** · **红黑树** |
| **④ 休眠唤醒** | 等待队列 | `wake_up()` |
| **⑤ 抢占与切换** | `context_switch` | **用户/内核抢占** |
| **⑥ RT 策略** | FIFO / RR | **软实时** |
| **⑦ syscall** | 调参接口 | **affinity · yield** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 多任务与调度器演进 | [notes/section-4.1-多任务与调度器演进.md](./notes/section-4.1-多任务与调度器演进.md) |
| 调度策略 | [notes/section-4.2-调度策略.md](./notes/section-4.2-调度策略.md) |
| Linux 调度算法 | [notes/section-4.3-Linux-调度算法.md](./notes/section-4.3-Linux-调度算法.md) |
| 休眠与唤醒 | [notes/section-4.4-休眠与唤醒.md](./notes/section-4.4-休眠与唤醒.md) |
| 抢占与上下文切换 | [notes/section-4.5-抢占与上下文切换.md](./notes/section-4.5-抢占与上下文切换.md) |
| 实时调度策略 | [notes/section-4.6-实时调度策略.md](./notes/section-4.6-实时调度策略.md) |
| 与调度相关的系统调用 | [notes/section-4.7-与调度相关的系统调用.md](./notes/section-4.7-与调度相关的系统调用.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| Linux 多任务？ | **抢占式** |
| 默认调度器？ | **CFS**（≥2.6.23）— `vruntime` 最小优先（红黑树最左） |
| `vruntime`？ | ≈ 实际时间 × (1024/权重)；权重高则涨得慢 |
| 默认 weight？ | nice=0 → **1024**（查表 `sched_prio_to_weight`；理论≈`1024/1.25^nice`） |
| `static_prio`？ | **`120 + nice`**，范围 **[100, 139]**；数字越小优先级越高 |
| fork 后 nice？ | **继承父进程**；`exec` **不改** nice |
| nice vs RT？ | nice 调 CFS 份额；RT **压过** 所有 CFS；nice **调不动** RT |
| `prio`？ | **全局标尺（越小越优先）**；CFS≈`static_prio`；RT=`99-rt_priority` |
| `rt_priority`？ | 用户 RT 优先级，**越大越优先**（与 nice、`prio` 方向都易混） |
| 六策略高低？ | **DEADLINE > FIFO/RR > NORMAL > BATCH > IDLE** |
| BATCH 是 RT？ | **否** — 仍是 CFS；IDLE **不受 nice** |
| HFT 三板斧？ | **`affinity` + `chrt` + 隔离核** |

---

## 相关章节

- 上一章：[../chapter-03-process-management/](../chapter-03-process-management/)
- 下一章：[../chapter-05-system-calls/](../chapter-05-system-calls/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
