# 4. 调度与唤醒 (Scheduler & Wakeups)

### `offcputime` — 过滤不可中断休眠

[Ch 6/13](../../chapter-13-applications/) 已介绍；本章强调 **状态过滤**：

| 状态 | 含义 |
|------|------|
| **`TASK_UNINTERRUPTIBLE` (D)** | 等 I/O、等锁 — **真实阻塞** |
| 可中断睡眠 | 可能含应用 `sleep()` — **噪音** |

**用法直觉：** Off-CPU 火焰图 **只看 D 状态** → 剔除主动 sleep，聚焦 **I/O/内核锁**。

```bash
# 具体过滤选项见 man offcputime-bpfcc
sudo offcputime-bpfcc -u -p $(pidof myapp) 30
```

### `wakeuptime`

记录 **执行唤醒的线程栈** + **被唤醒线程已阻塞多久**。

```bash
sudo wakeuptime-bpfcc 10
```

**回答：** **谁** 唤醒了沉睡线程 — 唤醒者侧视角。

### `offwaketime` — Off-Wake 火焰图 🔴

结合 **`offcputime` + `wakeuptime`**：

```
上半（倒置）  唤醒者栈  ───┐
                        ├── 交汇 = 唤醒点
下半          被阻塞者栈 ───┘
```

**价值：** 整条 **阻塞 → 唤醒** 链一目了然 — 「神秘系统卡顿」利器。

```bash
sudo offwaketime-bpfcc 30
```

**HFT incident：** 应用 `offcputime` 只有 `futex`/模糊栈 → **`offwaketime`** 追 **谁完成 I/O 并唤醒**。

---
