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

### 二、两大阵营 + 三套字段（一次性理顺）

Linux 调度两大类（再加 DEADLINE）：

| 阵营 | 策略 | 调度器 |
|------|------|--------|
| **普通分时** | `SCHED_OTHER`（及 BATCH / IDLE） | **CFS**（nice / `vruntime`） |
| **实时** | `SCHED_FIFO` / `SCHED_RR` | **RT 类**（独立队列，压过全部 CFS） |
| **限期** | `SCHED_DEADLINE` | DL 类（通常压过 RT） |

`task_struct` 优先级 **三兄弟**（外加 `rt_priority`）：

| 字段 | 角色 |
|------|------|
| **`static_prio`** | CFS **静态**源头（`120 + nice`） |
| **`normal_prio`** | **标准化基准** — 抹平 CFS / RT 两套标尺 |
| **`prio`** | **调度真正用来比较、抢占判断的最终值** |
| **`rt_priority`** | RT 用户优先级（FIFO/RR；CFS 不用） |

#### 公式总览（必背）

**CFS（NORMAL / BATCH / IDLE）：**

```
normal_prio = static_prio
prio        = normal_prio = static_prio
```

**RT（FIFO / RR）：**

```
normal_prio = 99 - rt_priority
prio        = normal_prio
```

**DEADLINE：** `prio` 固定为 **0** 量级（高于普通 RT 语义；走 DL 调度类）。

> 现代主线 CFS：**常态下 `prio == normal_prio`**。  
> O(1) 时代曾有交互 bonus，可使 `prio` 临时偏离 `normal_prio`；**CFS 已取消该动态奖励**。  
> 例外：RT **优先级继承（PI）** 等路径下，`effective_prio` 仍可能让运行中的 `prio` 暂时不同于 `normal_prio`（锁相关 boost）— 入门先记「无 PI 时相等」。

---

#### ① 普通 CFS → nice 体系

| 项 | 值 |
|----|-----|
| 控制参数 | **nice ∈ [−20, 19]** |
| `static_prio` | `120 + nice` → **[100, 139]** |
| `rt_priority` | **忽略**；改 nice **对 RT 无效** |
| 三字段 | `prio = normal_prio = static_prio` |
| 份额 | 用 `static_prio` 查表 **weight** → **`vruntime`**（选人看 vruntime，不看 static_prio 大小） |

---

#### ② 实时 RT → `rt_priority` 体系

| 项 | 值 |
|----|-----|
| 控制参数 | **`rt_priority`（约 0…99；越大越优先）** |
| nice / `static_prio` / `vruntime` | **不用** |
| 换算 | `normal_prio = 99 - rt_priority`；`prio = normal_prio` |
| 抢占 | 就绪即压过全部 CFS |

| `rt_priority` | `normal_prio` / `prio` |
|---------------|------------------------|
| 99（最高） | **0** |
| 50 | **49** |
| 0（最低） | **99** |

细节 → [§4.6](./section-4.6-实时调度策略.md) · [§4.7](./section-4.7-与调度相关的系统调用.md)

---

#### ③ 三兄弟逐个拆 + `prio` 全局标尺

| 字段 | 含义 |
|------|------|
| **`static_prio`** | CFS 专属基准；`120+nice`；不 renice 不变；**RT 忽略** |
| **`normal_prio`** | 统一转换层：CFS 抄 `static_prio`；RT 由 `rt_priority` 换算 |
| **`prio`** | **抢占/比较只看它**；**数字越小越优先** |

```
DEADLINE ── prio ≈ 0（最高档）
RT ──────── prio ∈ [0, 98]（随 rt_priority）
分界 ────── ≈ 99
CFS ─────── prio ∈ [100, 139]
```

→ 就绪 RT 的 `prio` 整体 **小于 100**（CFS 最小为 100）→ **天然抢占 CFS**。

##### 时序案例

| 案例 | 计算 | `prio` |
|------|------|--------|
| CFS nice=0 | static=120 → normal=120 → prio=120 | 120 |
| RT rt=50 | normal=`99-50`=49 → prio=49 | **49**（高于上者） |
| RT rt=99 | normal=0 → prio=0 | **0** |
| CFS nice=−20 | static=100 → prio=100 | 100 |

**自检：** rt=99 的 RT，`prio=0`；nice=−20 的 CFS，`prio=100`。  
`0 < 100` → **能抢占** 该 CFS。

##### 链路对照

```
CFS:  nice → static_prio → normal_prio → prio
         → 查表 weight → vruntime → 红黑树

RT:   rt_priority → normal_prio → prio → RT 队列
         （不走 CFS / vruntime）
```

##### 极简记忆

1. `static_prio`：CFS 源头（nice）；  
2. `normal_prio`：CFS/RT **统一转换层**；  
3. `prio`：内核 **最终比较值**；  
4. CFS：`prio = normal_prio = static_prio`；  
5. RT：`prio = normal_prio = 99 - rt_priority`。

| 谣言 | 真相 |
|------|------|
| `nice -20` ≈ 实时 | **否** — CFS 最小 `prio` 仍是 100 |
| `prio` 就是 `static_prio` | **否** — 仅 CFS 常态相等 |
| RT 用 nice 调 | **否** |
| `rt_priority` 越小越优先 | **否** — **越大越优先** |
| 现代 CFS 仍有 O(1) 式动态 bonus | **否** — 已取消；常态 `prio==normal_prio` |

```
普通世界（CFS）                    实时世界（独立类）
nice -20 ──────────► +19           rt_priority 99 ────► 0/1
static_prio 100 ───► 139           prio 0 ──────────► 99
```

| 权限直觉 | |
|----------|--|
| 普通用户 | 通常只能把 nice **调大** |
| root / 能力 | 才能 nice 负数、设 RT |

---

### 二½、nice → weight：初始权重从哪来？（CFS 必钉）

**一句话：** 进程初始权重 **由初始 nice 决定**；CFS 里 `nice` ↔ `weight` **一一映射**。

**核心结论：没有在运行时套初等公式实时算 weight。**  
工程上是 **预计算静态数组查表**；理论推导公式只用来说明「表从哪来」。

#### 理论公式（文档/注释语义，非运行时代码）

```
weight ≈ 1024 / (1.25 ^ nice)    ， nice ∈ [-20, 19]
```

内核注释（`kernel/sched/core.c`）：nice 每差 1，CPU 份额大约差 **~10%**（相对累计）；实现上用乘数 **1.25**。

| nice 变化 | 权重变化（理论） |
|-----------|------------------|
| nice **+1** | weight ÷ 1.25 |
| nice **−1** | weight × 1.25 |

验算（理论浮点 → 与表对照）：

| nice | 理论 | 内核表 `sched_prio_to_weight[]` |
|------|------|--------------------------------|
| 0 | 1024 / 1 = **1024** | **1024** |
| 1 | 1024 / 1.25 = 819.2 | **820**（取整，≠ 819） |
| −1 | 1024 × 1.25 = 1280 | **1277**（与浮点不完全一致） |

→ **必须以数组为准**；不要自己写 `1024 / pow(1.25, nice)` 当内核行为。

#### 为何查表、不实时算？

1. 内核路径 **忌随便用浮点**（开销 / FPU 状态）；  
2. nice 只有 **40** 个取值（−20…19）；  
3. 开发期用公式算好整数，**编译期写死**进数组。

现代符号名：**`sched_prio_to_weight[40]`**（老资料常简称 `prio_to_weight`）。

```c
/* 摘自 linux-7.1.5 kernel/sched/core.c — 勿用网上被截断的错误表 */
const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};
```

| nice | 含义 | weight |
|------|------|--------|
| **−20** | 最高 | **88761** |
| **0** | 默认 | **1024** |
| **+19** | 最低 | **15** |

> 网上常见错误表把 −20 写成 `8876`、把 0 写成 `102` — **少一位**。以本仓库对照的源码为准。

#### 下标怎么对应？

```
nice
  → static_prio = 120 + nice          （NICE_TO_PRIO；DEFAULT_PRIO=120）
  → index = static_prio - 100         （= 20 + nice）
  → weight = sched_prio_to_weight[index]
```

| nice | static_prio | index |
|------|-------------|-------|
| −20 | 100 | 0 |
| 0 | 120 | 20 |
| 19 | 139 | 39 |

#### 配套：`sched_prio_to_wmult[]`（规避除法）

概念上：

```
vruntime += Δt × (1024 / weight)
```

工程上用预计算逆元 **定点乘法**（注释：`2^32 / weight`）：

```
vruntime += delta_exec * sched_prio_to_wmult[index] >> 32;
```

→ [§4.3 `vruntime`](./section-4.3-Linux-调度算法.md)

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

#### `static_prio`：静态优先级（CFS 优先级源头 · 完整钉死）

`task_struct` 成员；**普通 CFS 进程优先级的原始基准值**。

**死记：**

```
static_prio = 120 + nice          /* NICE_TO_PRIO；DEFAULT_PRIO = 120 */
nice        = static_prio - 120   /* PRIO_TO_NICE */
```

| | 范围 |
|--|------|
| nice | **[−20, 19]** |
| **static_prio** | **[100, 139]** |

| nice | static_prio | 含义 |
|------|-------------|------|
| **−20** | **100** | CFS **最高**权重侧（数字最小） |
| **0** | **120** | 默认 |
| **+19** | **139** | CFS **最低**权重侧（数字最大） |

> 内核约定：**prio / static_prio 数字越小，优先级越高**。  
> RT 占约 **0…99**；CFS 落在 **100…139**（`MAX_RT_PRIO=100`）。

##### 「静态」= 不自动变

- 创建时从父进程 **继承**（随 nice 一起）；  
- **不**主动 `nice()` / `setpriority()` / `renice` → **整生命周期固定**；  
- **`exec()` 不改** `static_prio`。

##### 在 CFS 链路里的位置

```
nice ↔ static_prio → 查表 weight → vruntime → 红黑树
```

1. 用户改 nice → 内核改 **`static_prio`**（再刷新 weight）；  
2. **调度器不直接拿 `static_prio` 比谁先跑** — 它是 **中间存储 / 查表下标源头**；  
3. 需要权重时：

```c
index = static_prio - 100;   /* = 20 + nice */
weight = sched_prio_to_weight[index];
```

真正选人看的是 **`vruntime`**；`static_prio` 只通过 weight **间接**改变 `vruntime` 上涨速度。

例：A(nice=0 → 120 → weight=1024) 与 B(nice=10 → 130 → weight 很小) — 调度器 **不比 120 vs 130**，只比两者 **`vruntime`**。

##### 和另外两个字段

| 字段 | 谁用 | 要点 |
|------|------|------|
| **`static_prio`** | CFS 基准 | nice 换算；长期不变 |
| **`rt_priority`** | RT FIFO/RR | 0…99；与 nice/`static_prio` **无关** |
| **`prio`** | 全局比较 | CFS：`prio = static_prio`；RT：`prio = 99 - rt_priority`；**越小越优先** |

##### 误区

| 说法 | 对错 |
|------|------|
| 调度器直接靠 `static_prio` 决定谁先运行 | ❌ 选人看 **`vruntime`** |
| `prio` 永远等于 `static_prio` | ❌ 仅 CFS 常态；RT 另算 |

##### 一句话背诵

`static_prio = 120 + nice`，PCB 里的 **固定基准**；本身不直接分 CPU，作用是 **查表得 weight**，最终影响 **`vruntime` 增速**。

##### 自检：`renice` 改了什么？

内核底层改的是 **`static_prio`**（由新 nice 换算），并据此更新 **weight**；进而改变之后的 **`vruntime` 增速**。不是直接改 `vruntime`，也不是改 `rt_priority`。

#### 两条易混：`static_prio` vs `weight`

| 名字 | 是什么 | 怎么来 |
|------|--------|--------|
| **`static_prio`** | 静态优先级数字（100…139） | `120 + nice` |
| **`weight`** | CFS 真正用来算份额的权重 | `index = static_prio - 100` → **`sched_prio_to_weight[index]`** |

```
nice → static_prio → index → 查表 weight →（wmult）→ vruntime 增速
```

#### 误区清单

| 说法 | 对错 |
|------|------|
| 运行时套 `1024/1.25^nice` 就等于内核 | ❌ 取整与表不完全一致 |
| 普通 `SCHED_OTHER` weight 会自动升降 | ❌ **不改 nice 则不变** |
| 表可以随便缩写成 8876 / 102 | ❌ 以源码全精度为准 |

#### 背诵

1. **理论：** `weight ≈ 1024 / 1.25^nice`；nice+1 → ÷1.25。  
2. **工程：** `sched_prio_to_weight[]` 查表；`wmult` 做定点乘。  
3. **`static_prio = 120 + nice`** ∈ [100, 139]；越小越优先。  
4. shell 默认 0 → fork 继承 → exec 不改；要改就显式 `nice`/`setpriority`。

#### 自检

父 nice=5，`fork` 后子进程 `exec` 新程序 → **子初始 nice 仍是 5**。

---

### 三、六种调度策略完整汇总

三大阵营：

| 阵营 | 策略 |
|------|------|
| **1. CFS 普通分时** | `SCHED_OTHER`/`NORMAL` · `SCHED_BATCH` · `SCHED_IDLE` |
| **2. RT 实时** | `SCHED_FIFO` · `SCHED_RR` |
| **3. Deadline 限期** | `SCHED_DEADLINE`（硬实时扩展，入门可浅看） |

#### 逐行精讲

##### 1. `SCHED_OTHER`（=`SCHED_NORMAL`）

| | |
|--|--|
| 类 | **标准 CFS** |
| 说明 | Linux **默认**；shell、应用、多数后台服务都是它 |
| 机制 | nice → `static_prio` → weight → `vruntime` / 红黑树 |
| 注意 | 无实时优先级；**抢不过** RT / DEADLINE |

新内核正式名常写 **`SCHED_NORMAL`**，旧别名 **`SCHED_OTHER`** 仍保留。

##### 2. `SCHED_BATCH`

| | |
|--|--|
| 类 | **CFS 变体**（仍是 CFS，**不是**实时） |
| 说明 | 批量 / 离线计算：吞吐优先、少交互 |
| 特性 | **减少唤醒抢占优待**；短暂休眠不会像交互任务那样被优待 |
| 仍用 | `vruntime`、nice / weight；**不适合**桌面交互 |

##### 3. `SCHED_IDLE`

| | |
|--|--|
| 类 | **极低** CFS 任务 |
| 说明 | **只有 CPU 完全空闲**（没有 NORMAL/BATCH 可跑）才给 CPU |
| 对比 | 哪怕 nice=19 的 NORMAL，也 **高于** IDLE |
| 注意 | **不再受 nice 控制**；典型：闲时爬虫、碎片整理一类 |

##### 4. `SCHED_FIFO`（RT）

| | |
|--|--|
| 类 | RT 实时 |
| 时间片 | **无**；同优先级不轮转 |
| 何时让出 | ① 阻塞 ② `sched_yield()` ③ 被 **更高 `rt_priority`** 抢占 |
| 同优先级 | 先就绪先跑；不主动让出就一直占 CPU |

##### 5. `SCHED_RR`（RT）

| | |
|--|--|
| 类 | RT 实时 |
| 相对 FIFO | 同优先级加 **固定时间片**；用完排到同优先级队尾 |
| 抢占 | 更高 RT 仍可抢占 |

> **FIFO / RR：** 用 **`rt_priority`（约 0…99）**，与 nice **彻底无关**。→ [§4.6](./section-4.6-实时调度策略.md)

##### 6. `SCHED_DEADLINE`

| | |
|--|--|
| 类 | 现代 **限期实时**（EDF：最早截止优先） |
| 说明 | 不靠固定优先级；任务声明 **运行时长 / 周期 / 截止时间**；优先跑 **最先到截止** 的 |
| 场景 | 工控、音视频低延迟等；入门内核可先记名字，不必深挖 |

#### 全局优先级顺序（高 → 低）

```
SCHED_DEADLINE
  > SCHED_FIFO / SCHED_RR（RT）
  > SCHED_NORMAL（OTHER）
  > SCHED_BATCH
  > SCHED_IDLE
```

只要有就绪的 **DEADLINE / RT**，**所有 CFS 普通进程全部靠边**。

#### 易混点

| 说法 | 对错 |
|------|------|
| OTHER / BATCH / IDLE 都走 CFS（`vruntime`） | ✅ |
| FIFO / RR 独立 RT 队列，不走 CFS 红黑树 | ✅ |
| RT 可抢 CFS；CFS **抢不了** RT | ✅ |
| `SCHED_BATCH` 是实时策略 | ❌ 仍是 CFS |
| `SCHED_IDLE` 用 nice 精细调 | ❌ **不受 nice 控制** |

#### 和 `sched_setscheduler` 串联

```
task_struct ──sched_setscheduler()──► 设 policy
  ├─ NORMAL / BATCH ──► nice / static_prio / CFS vruntime
  ├─ IDLE ───────────► 极低 CFS（不靠 nice）
  ├─ FIFO / RR ──────► rt_priority → RT 队列
  └─ DEADLINE ───────► runtime / period / deadline 参数
```

接口细节 → [§4.7](./section-4.7-与调度相关的系统调用.md)

FIFO / RR 带宽节流等 → [§4.6](./section-4.6-实时调度策略.md)

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
| `exec` 不换 PID | `exec` **不改** nice/权重（除非程序自己再调）；见上文「nice → weight」 |

---

### 五、开发场景提示

| 场景 | 建议 |
|------|------|
| **Rust/网络 IO 服务** | 默认 **CFS 通常够用且更安全**；乱上 `SCHED_FIFO` 易饿死系统线程、看起来像「整机卡死」 |
| **HFT / 超低延迟** | 才认真考虑 **隔离核 + `SCHED_FIFO` + 中断亲和 + 内核抢占模型**；控制面仍留 CFS |
| 调参入口 | `nice` / `chrt` / `sched_setscheduler` / affinity → [4.7](./section-4.7-与调度相关的系统调用.md) |

**HFT：** 行情/撮合热路径用 RT+绑核；日志、监控、非关键路径留 CFS。

→ [07 TLPI](../../../../03-linux-userspace-api/) · [4.3 CFS](./section-4.3-Linux-调度算法.md) · [4.6 RT](./section-4.6-实时调度策略.md)

### 常见陷阱

1. 混淆 SCHED_OTHER 和 SCHED_FIFO——OTHER 是 CFS 管的普通分时，FIFO 是 RT 调度器管的实时
2. 以为 nice 值 -20 到 19 对应优先级 -20 到 19——内部映射为 static_prio = 120 + nice，范围 [100, 139]
3. 在 RT 策略下以为 nice 还有效——RT 策略看 rt_priority (1-99)，不看 nice

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 六个调度策略（SCHED_OTHER/BATCH/IDLE/FIFO/RR/DEADLINE）各自适用什么场景？

<details><summary>答案</summary>

OTHER：普通分时进程（默认）。BATCH：CPU 密集型批处理（低交互）。IDLE：极低优先级后台任务。FIFO：实时 FIFO，无时间片。RR：实时轮转，有时间片。DEADLINE：基于 EDF，需指定 runtime/deadline/period，优先级最高。HFT 用 FIFO。

</details>

**Q2.** nice 值到 CPU 权重的映射是怎么实现的？

<details><summary>答案</summary>

nice [-20,19] → static_prio = 120 + nice → 查 sched_prio_to_weight[] 表。nice 0 = weight 1024，nice +5 = 335，nice -5 = 3121。每差 1 级 nice，权重比约 1.25。两个进程 A(nice=0, weight=1024) B(nice=5, weight=335) 的 CPU 比例 ≈ 1024:335 ≈ 3:1。

</details>

**Q3.** `SCHED_FIFO` 的 `rt_priority` 怎么设置？范围是什么？

<details><summary>答案</summary>

通过 `sched_setscheduler(pid, SCHED_FIFO, &param)` 设置，`param.sched_priority` 范围 1-99（0 表示非 RT）。数字越大优先级越高。HFT 通常设 99（最高），配合 `isolcpus` 独占 CPU。注意需要 `CAP_SYS_NICE` 权限。`/proc/sys/kernel/sched_rt_runtime_us` 默认 950000 限制 RT 占用 95% CPU 时间，HFT 可设 -1 禁用。

</details>

</details>


> ↔ [ULK Ch7 §2 调度策略与抢占](../../../../20-linux-kernel-deep/chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md)
---
