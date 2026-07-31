## ② 调度策略 · Scheduling Policy

> 承接 [4.1 抢占与演进](./section-4.1-多任务与调度器演进.md)。  
> 策略回答两件事：**谁优先跑**、**可以跑多久（份额）**。

Linux 把任务隔成两大阵营（两套调度类，**不要混优先级标尺**）：

| 阵营 | 调度器 / 策略 | 谁用 |
|------|----------------|------|
| **普通分时** | **CFS**（默认 `SCHED_OTHER`） | 绝大多数程序 |
| **实时** | **`SCHED_FIFO` / `SCHED_RR`** | 高优先级；**压过全部 CFS** |

连贯顺序见本章 [README 推荐阅读](../README.md)。

---

### 一、I/O-bound vs CPU-bound

| 类型 | 典型行为 | 调度诉求 | 例子 |
|------|----------|----------|------|
| **I/O-bound** | 常阻塞等盘/网/锁；多数时间休眠 | 醒来 **立刻** 拿 CPU → **低延迟** | Web、DB、网络服务、多数 Rust 异步服务 |
| **CPU-bound** | 长时间占满核，极少睡 | **稳定份额**，少被无谓打断 → **吞吐** | 编码、数值计算、大型编译 |

#### CFS 折中：`vruntime`（虚拟运行时间）

| 规则 | 效果 |
|------|------|
| 每任务记账 **`vruntime`** | 「相对跑了多少」 |
| 常休眠（偏 I/O） | `vruntime` **涨得慢** → 「欠账」少 |
| 一直狂跑（偏 CPU） | `vruntime` **涨得快** → 更容易被换下 |
| 选人 | **永远优先 `vruntime` 最小**（红黑树最左）→ 见 [4.3](./section-4.3-Linux-调度算法.md) |

> **通俗：** 经常睡觉的程序醒来优先跑；一直占着 CPU 的程序「欠账」堆高，更容易让出 CPU。  
> 因此默认 CFS 对 **网络 / 交互** 相对友好 — **不必** 一上来就给业务线程上 RT。

---

### 二、两套优先级（高频踩坑 · 绝对分清）

| 体系 | 范围 | 方向 | 作用 | 无效对象 |
|------|------|------|------|----------|
| **nice**（CFS） | −20 … +19（默认 0） | **越小份额越大** | 调 **权重 / CPU 比例**；**不能**靠 nice「抢占」别人出 CFS 世界 | **对 RT 任务完全无效** |
| **RT prio**（FIFO/RR） | 1 … 99 | **越大越优先** | RT 压过 **所有 CFS**；高 RT 压低 RT | nice 调不动它 |

```
普通世界（CFS）                    实时世界（独立类）
nice -20 ──────────► +19           RT 99 ──────────► 1
  份额大              份额小         压过一切 CFS      仍压过 CFS
```

| 权限直觉 | |
|----------|--|
| 普通用户 | 通常只能把 nice **调大**（更「谦让」） |
| root / 能力 | 才能 nice 负数、设 RT 策略 |

| 谣言 | 真相 |
|------|------|
| `nice -20` ≈ 实时 | **否** — 再负的 nice 也压不过任意可运行 RT |
| nice 数字方向 = RT 数字方向 | **否** — **两套标尺，方向相反** |

---

### 二½、nice → weight：初始权重从哪来？（CFS 必钉）

**一句话：** 进程初始权重 **由初始 nice 决定**；CFS 里 `nice` ↔ `weight` **一一映射**。

#### 区间与查表（不是实时公式算出来的）

| nice | 含义 | weight（内核表） |
|------|------|------------------|
| **−20** | 最高权重 | **88761** |
| **0** | 默认 | **1024** |
| **+19** | 最低权重 | **15** |

nice 范围：**[−20, 19]**。  
内核用静态数组 **`sched_prio_to_weight[]`**（40 项，对应每个 nice）查表；本机树见 `kernel/sched/core.c`。

> 常见笔误：把 −20 的权重写成 `8876` — 源码是 **88761**。

#### 新进程 nice 从哪继承？

| 场景 | 结果 |
|------|------|
| shell 跑 `./a.out` | shell 默认 nice=0 → `fork` 子进程 **继承** → 通常 **nice=0, weight=1024** |
| `fork()` | 子进程初始 nice = **父进程的 N** |
| `exec()` | **不改 nice** — 只换程序镜像，调度属性保留 |
| `nice -n -5 ./app` | 启动时直接设初始 nice |
| `renice` / `nice()` / `setpriority()` | 运行中显式改 |

```
父 nice = N
  fork → 子 nice = N
  exec → 子 nice 仍是 N（除非程序自己再改）
```

#### 两条易混的内核值

| 名字 | 是什么 | 怎么来 |
|------|--------|--------|
| **`static_prio`** | 静态优先级；**不改 nice 就长期不变** | `static_prio = 120 + nice`（即 `NICE_TO_PRIO`；`DEFAULT_PRIO=120`） |
| **`weight`** | CFS 真正用来算份额的权重 | 用 `static_prio` 去索引 **`sched_prio_to_weight[]`** |

例：nice=0 → `static_prio=120` → weight=1024；nice=−20 → 100 → 88761；nice=19 → 139 → 15。

```
初始 nice → static_prio → 查表 → weight（CFS）→ 影响 vruntime 增速
```

```
vruntime += Δt × (1024 / weight)
```

weight 越大 → 同样 Δt，`vruntime` 涨得越慢 → 分到更多 CPU。细节 → [§4.3](./section-4.3-Linux-调度算法.md)

#### 误区：权重会自动升降？

| 说法 | 对错 |
|------|------|
| 普通 `SCHED_OTHER` 任务 weight 会自动动态升降 | ❌ |
| **不改 nice，weight 终身不变**（对普通 CFS 任务） | ✅ |

组调度、带宽控制、`SCHED_IDLE` 等另说；**普通进程不会自己改 weight**。

#### 背诵链路

1. shell 默认 nice=0；  
2. `fork` 继承父 nice；`exec` 不改；  
3. nice → `static_prio` → `sched_prio_to_weight[]` → weight；  
4. CFS 用 weight 算 `vruntime`；  
5. 要改权重：必须显式 `nice` / `setpriority` / `renice`。

#### 自检

父 nice=5，`fork` 后子进程 `exec` 新程序 → **子初始 nice 仍是 5**（继承自父；`exec` 不改调度属性）。

---

### 三、策略常量速查

| 策略 | 类 | 说明 |
|------|-----|------|
| **`SCHED_OTHER` / `SCHED_NORMAL`** | CFS | **默认**分时 |
| **`SCHED_BATCH`** | CFS 变体 | 更偏吞吐、少交互优待 |
| **`SCHED_IDLE`** | 极低 | 几乎只吃空闲 |
| **`SCHED_FIFO`** | RT | 同 prio **无时间片**；跑到阻塞 / yield / 被更高 RT 抢 | 
| **`SCHED_RR`** | RT | 同 prio **有时间片**，用完排队尾 |
| **`SCHED_DEADLINE`** | 现代扩展 | 书版可能略；生产按需 |

FIFO / RR 细节与带宽节流 → [4.6](./section-4.6-实时调度策略.md)

---

### 四、串回 `task_struct` / runqueue

```
fork → task_struct（内存里带着：policy、nice/权重、vruntime、RT prio…）
         │
         ▼
就绪 → 进对应调度类的 runqueue / CFS 树
         │
         ▼
调度器按「策略 + nice权重/vruntime 或 RT prio」选下一个
         → 抢占 / 上下文切换（4.1 / 4.5）
```

| 身份链 | 调度链 |
|--------|--------|
| PID = 是谁（[§3.8](../../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)） | policy + nice/RT = **怎么被挑上 CPU** |
| `exec` 不换 PID | `exec` **一般也不改** 你已设的调度策略（除非程序自己再 `sched_setscheduler`） |

---

### 五、开发场景提示

| 场景 | 建议 |
|------|------|
| **Rust/网络 IO 服务** | 默认 **CFS 通常够用且更安全**；乱上 `SCHED_FIFO` 易饿死系统线程、看起来像「整机卡死」 |
| **HFT / 超低延迟** | 才认真考虑 **隔离核 + `SCHED_FIFO` + 中断亲和 + 内核抢占模型**；控制面仍留 CFS |
| 调参入口 | `nice` / `chrt` / `sched_setscheduler` / affinity → [4.7](./section-4.7-与调度相关的系统调用.md) |

**HFT：** 行情/撮合热路径用 RT+绑核；日志、监控、非关键路径留 CFS。

→ [07 TLPI](../../../../07-The-Linux-Programming-Interface/) · [4.3 CFS](./section-4.3-Linux-调度算法.md) · [4.6 RT](./section-4.6-实时调度策略.md)

---
