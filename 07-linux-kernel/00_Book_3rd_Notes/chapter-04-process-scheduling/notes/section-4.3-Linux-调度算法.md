## ③ Linux 调度算法 · CFS

**CFS = Completely Fair Scheduler，完全公平调度器**  
Linux **2.6.23** 起，默认调度 **普通分时进程**（`SCHED_OTHER` / `SCHED_NORMAL`）。淘汰了古老的 **O(1)** 调度器。

---

### `SCHED_NORMAL` 完整正确链路（背诵）

```
nice → static_prio → 查表 weight
         → 运行时累加 vruntime
         → 就绪实体挂红黑树（key = vruntime）
         → 永远挑最左（vruntime 最小）上 CPU
```

| 步骤 | 作用 |
|------|------|
| **nice → `static_prio`** | `static_prio = 120 + nice` ∈ [100,139]；存在 `task_struct` 的 **基准**；不 renice 就不变。调度器 **不直接** 用它分时间 |
| **`static_prio` → weight** | 查 `sched_prio_to_weight[]`；按权重 **比例** 分 CPU |
| **weight → `vruntime`** | `vruntime += Δt × (1024/weight)`；公平 **账本** — 不比谁物理跑更久，比虚拟时间是否均衡 |
| **`vruntime` → 红黑树** | 就绪 `sched_entity` 入 `cfs_rq` 树；`rb_first()` = 下一个 |

细节（查表 / `static_prio` / RT 边界）→ [§4.2](./section-4.2-调度策略.md)

---

### ❌ 修正：不是「只有时间片耗尽才调度」

CFS **没有** 传统固定长度时间片作为唯一触发器。换人主要两类：

| 类型 | 何时 |
|------|------|
| **1. 周期性调度** | 时钟 tick 更新 `vruntime`；判断当前任务是否已跑满 **应得份额** → 放回树，再选最小 `vruntime` |
| **2. 唤醒抢占** | 休眠任务醒来；若其 `vruntime` 比当前 **小足够多** → **立刻抢占**（交互流畅的核心） |

进程越多，每人分到的时间越短（目标延迟窗口内均分）— **没有固定 N ms 片**。

---

### 一句话理念

老式调度：固定时间片轮流。  
CFS：假想完美 CPU，按 **权重** 瓜分时间；`vruntime` 记账 + 红黑树选最小。

例：A 权重 1024，B 权重 512 → A≈2/3，B≈1/3。

---

### 核心对象逐个拆

| 概念 | 是什么 |
|------|--------|
| **`sched_entity`** | 调度实体。嵌在 `task_struct` 里。CFS **不直接操作** `task_struct`，统一操作 `sched_entity` |
| **`cfs_rq`** | **每 CPU 一条** CFS 就绪队列 |
| **`vruntime`** | 实际时间按权重标准化后的记账 |
| **红黑树** | key=`vruntime`；最左 = 最小 |

#### `vruntime` 公式

```
vruntime += Δt × (1024 / weight)
```

工程上常用 **`sched_prio_to_wmult[]`** 定点乘（见 [§4.2](./section-4.2-调度策略.md)）。

| 权重 | 同样物理 Δt | 效果 |
|------|-------------|------|
| **大**（nice 低） | `vruntime` 涨得 **慢** | 更能占 CPU |
| **小**（nice 高） | 涨得 **快** | 更易让出 |

```
权重高（nice 低）──► vruntime 涨得慢 ──► 更常被选中
权重低（nice 高）──► vruntime 涨得快 ──► 更少被选中
```

---

### 选谁跑

1. `rb_first`：最左 = `vruntime` 最小；  
2. 运行中持续累加 `vruntime`；  
3. 周期达标或被抢占 → 放回树；再选最小。

| 规则 | 实现 |
|------|------|
| 选最小 `vruntime` | 最「欠账」 |
| 复杂度 | **O(log n)** |

```
        红黑树（按 vruntime）
    最左 = 最小 vruntime ──► 下次运行
```

→ **[Ch6 §6.5](../../chapter-06-kernel-data-structures/notes/section-6.5-二叉树.md)**

---

### 何时换人（两大场景）

#### 场景 1：周期性调度（tick）

进程在跑 → 每次时钟中断更新当前 `vruntime` → 判断是否已达 **应得份额** → 是则 `need_resched` → 放回红黑树 → 再选最小。  
这是「类似旧时间片耗尽」的感觉，但 **长度随就绪任务数变化**。

#### 场景 2：唤醒抢占（交互核心）

CPU 跑着 P；Q 从休眠唤醒（键盘、socket…）。  
若 `Q.vruntime` 比 `P` **小过阈值** → **直接抢占 P，Q 立刻上 CPU**。

例：后台编译（nice 高，`vruntime` 涨得快）占着 CPU；敲键盘唤醒 shell（`vruntime` 更小）→ shell 抢占 → 终端立刻响应。  
这是 CFS 交互手感好、相对旧 O(1) 的关键改进。细节 → [§4.5](./section-4.5-抢占与上下文切换.md)

---

### 边界前提（整套 CFS 何时生效）

以下 **全部** 满足，nice/`static_prio`/weight/`vruntime`/红黑树才主导：

1. 策略 ∈ **`SCHED_NORMAL` / `BATCH` / `IDLE`**（CFS 家族）；  
2. **当前 CPU 没有就绪的 RT（FIFO/RR）或 DEADLINE**。

有就绪实时/限期任务 → **直接压过全部 CFS**，本套逻辑暂时让路。→ [§4.2 六策略](./section-4.2-调度策略.md)

---

### 背诵流程

1. `fork` 继承 nice → `static_prio`；  
2. 查表得 weight；  
3. 运行累加 `vruntime`；  
4. 就绪挂 `cfs_rq` 红黑树；  
5. 选最小 `vruntime`；  
6. 换人：① 周期份额达标 ② 唤醒抢占；  
7. 有就绪 RT/DEADLINE → CFS 全体靠边。

#### 自检

A(nice=0, weight=**1024**)、B(nice=5, weight=**335**，表值；勿写成 312) 长期同时就绪：  
物理 CPU 比 ≈ **1024 : 335** ≈ **75% : 25%**（约 **3 : 1**）。

---

### 必须配套理解

#### 调度周期（`sched_latency` 一类）

目标延迟窗口内就绪任务都至少轮到一次；人越多每人越短。  
`/proc/sys/kernel/sched_*` 可调 — 先懂语义再碰数字。

#### 和 RT 的边界

```
有可运行 RT/DEADLINE？──是──► 它们接管（CFS 靠边）
        │否
        ▼
      CFS 选最小 vruntime
```

---

### 与「时间片」的关系

| 旧直觉 | CFS |
|--------|-----|
| 每人固定 N ms | **没有** 固定绝对片长 |
| 「片用完才切」 | ❌ — 还有 **唤醒抢占**；周期检查看的是 **应得份额** |

---

### 串联：nice / task_struct / 系统调用

| 用户操作 | 底层效果 |
|----------|----------|
| `nice()` / `setpriority()` | 改权重 → 改 `vruntime` 增速 |
| `task_struct` 内嵌 `sched_entity` | 带着权重、`vruntime` |
| `current` | 当前任务（含调度实体） |

→ [§4.7](./section-4.7-与调度相关的系统调用.md) · [Ch5 `current`](../../chapter-05-system-calls/notes/section-5.5-系统调用上下文.md)

---

### 组调度 / cgroup（知道存在即可）

| 概念 | 用途 |
|------|------|
| **组调度** | 容器/服务按组分 CPU 份额 |
| **HFT** | 热路径常 **绑独立核**；进 RT 后 CFS 公平性不再保护它们 |

**HFT：** 热路径若进 RT，抖动源常变成中断/softirq/同核争用/锁，而不是 nice。

→ **Ch 6** 红黑树 · [4.5 抢占](./section-4.5-抢占与上下文切换.md) · [15 SysPerf](../../../../19-systems-performance/)

---
