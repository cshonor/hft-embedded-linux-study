## ① Unix 的历史 · History of Unix

| 事实 | 说明 |
|------|------|
| **起源** | **1969** · 贝尔实验室 · **Dennis Ritchie**、**Ken Thompson** |
| **成功因素** | 见下表 |

| Unix 优势 | 含义 |
|-----------|------|
| **设计简洁** | 仅 **~几百个系统调用** — 接口少而稳 |
| **一切皆文件** | 设备、socket、管道… 统一 **open/read/write** |
| **C 语言实现** | **可移植** — 换硬件主要重编译内核 |
| **极快进程创建** | 独特 **`fork()`** — 复制地址空间语义 |
| **稳健 IPC** | 管道、信号等 **简单原语** |

> **本篇的 v6.6 实证材料**（全部抓自 `torvalds/linux@v6.6`）：
> `arch/x86/entry/syscalls/syscall_64.tbl`（14955 B，x86_64 系统调用表全表）
> `include/uapi/asm-generic/unistd.h`（30495 B）
> `arch/x86/Kconfig` / `kernel/Kconfig.preempt` 的 **v6.6 / v6.11 / v6.12 / v6.13 / v6.14** 五版
> `rust/Makefile` 的 **v5.19 / v6.0 / v6.1** 三版
> `Makefile`（v6.6）
>
> ⚠️ 原笔记说「Unix 只有**几百个**系统调用」。这个说法对，但太模糊——
> 下面的 §1 用 v6.6 的 syscall 表把它精确到个位数，并且顺带挖出
> **一段内核自己写下来承认的"历史设计错误"**（§2）。

#### 时间线（考据锚点）

```
1969  Thompson 在 PDP-7 上写 Unix（汇编）
1973  Ritchie 用 C 重写 —— 跨硬件移植的门被打开
1977  Berkeley Software Distribution (BSD) 分支起步
1983  System V（AT&T 商业版）vs BSD —— 「Unix 战争」
1988  POSIX 诞生 —— 停战方案：只标准化 API，不管实现
1991  Linux 以 GPL 重做类 Unix 内核 —— 吸收两者遗产
```

| 分叉 | 留给 Linux 的遗产 |
|------|------------------|
| System V | `SysV IPC`（shm/sem/msg）、init 体系 |
| BSD | sockets 网络栈、`mmap`、virtual memory 布局 |
| POSIX | 今天的可移植性契约——Linux syscall 表基本按 POSIX 覆盖设计 |

#### 五条优势的内核机制对应

| 哲学 | 内核里的落点 | 现代演化 |
|------|--------------|----------|
| 一切皆文件 | VFS 的 `file_operations` 抽象（Ch 13） | io_uring 用**队列**而非 fd 流——抽象开始让位于延迟 |
| C 实现 | 内核至今 C 为主（+少量 Rust，6.1+） | Rust for drivers 正在写入主线 |
| fork 语义 | 写时拷贝 COW（[3.4](../../chapter-03-process-management/notes/section-3.4-进程创建与写时拷贝.md)） | `posix_spawn`/`vfork` 是后辈对 fork 成本的补丁 |
| 少 syscall | `read/write/mmap` 覆盖 90% 场景 | io_uring 一个入口复用百种操作 |

---

## §1 ⭐「几百个系统调用」的精确数字（v6.6 实证）

原笔记写「仅 **~几百个**系统调用」。这句话是对的，但可以精确到个位数。
直接数 `arch/x86/entry/syscalls/syscall_64.tbl`（v6.6）：

| 指标 | 数值 | 说明 |
|------|------|------|
| 表内条目总数 | **401** | 含 x32 专属 |
| **64 位可用** | **365** | = `common` 317 + `64` 专属 48 |
| x32 专属 | 36 | 号码 512–547 |
| 最大号码 | **547** | 但中间有洞，见 §2 |
| 号码空间上界 | 0–453（64 位段） | `__NR_syscalls 453`（`unistd.h:827`） |
| **号码空洞** | **89 个** | ⭐ 连续一段：335–423 |
| 未实现存根 | 1 个 | `156 _sysctl` → `sys_ni_syscall` |

⭐ **所以准确的答案是：v6.6 的 x86_64 上大约 365 个 64 位系统调用。**

### 1.1 「几百个」这个量级意味着什么

做个横向对比就很直观：

| 系统 | 接口面 |
|------|--------|
| Linux v6.6（x86_64） | **365** 个 syscall |
| 一个典型 C 库（glibc） | 导出 **2000+** 个符号 |
| 一个桌面应用的依赖树 | 动辄数百个动态库、数万个符号 |

**内核对用户的承诺面，比它之上的任何一层都小一个数量级。**
这就是 Unix「设计简洁」这条优势的量化表达——它不是口号，是可数的。

### 1.2 表本身长什么样

```
#
# 64-bit system call numbers and entry vectors
#
# The format is:
# <number> <abi> <name> <entry point>
#
# The __x64_sys_*() stubs are created on-the-fly for sys_*() system calls
#
# The abi is "common", "64" or "x32" for this file.
#
0	common	read			sys_read
1	common	write			sys_write
2	common	open			sys_open
3	common	close			sys_close
...
334	common	rseq
424	common	pidfd_send_signal
425	common	io_uring_setup
426	common	io_uring_enter
427	common	io_uring_register
...
453	64	map_shadow_stack
```

注意 `abi` 这一列有三类：
- **`common`**：64 位和 x32 共用
- **`64`**：仅 64 位（如 `453 map_shadow_stack`）
- **`x32`**：仅 x32 ABI

---

## §2 ⭐⭐ 号码表里的历史化石：335–423 那 89 个空洞

这是本篇最有意思的发现——**Unix/Linux 的历史债，在系统调用号码表上留下了肉眼可见的疤痕，
而且内核开发者把原因直接写在了表里。**

### 2.1 空洞的形状

把 0–453 全扫一遍，缺失的号码**只有一段连续的**：

```
  0 ────────────── 334    335 ────────────── 423     424 ──────── 453
  ┌───────────────────┐   ┌─────────────────────┐   ┌───────────────┐
  │  全部在用          │   │  89 个号码，空的      │   │  全部在用      │
  │  最后一个是 rseq   │   │  （注释禁止使用）     │   │  从 pidfd 起  │
  └───────────────────┘   └─────────────────────┘   └───────────────┘
```

### 2.2 内核自己写的注释（v6.6 原文）

```c
# don't use numbers 387 through 423, add new calls after the last
# 'common' entry
#
# Due to a historical design error, certain syscalls are numbered differently
# in x32 as compared to native x86_64.  These syscalls have numbers 512-547.
# Do not add new syscalls to this range.  Numbers 548 and above are available
# for non-x32 use.
#
# This is the end of the legacy x32 range.  Numbers 548 and above are
# not special and are not to be used for x32-specific syscalls.
```

⭐⭐ 三条信息，逐条读：

**① "Due to a historical design error"**
内核源码**明文承认这是一个历史设计错误**。x32 ABI 里有一批 syscall 的号码
与原生 x86_64 不同，被塞在 512–547。这是 Linux 源码里少见的、直接自嘲的注释。

**② "don't use numbers 387 through 423"**
明确划为禁区。注意注释只提 387–423，**但实际空洞从 335 就开始了**
（335–386 这段的差异注释没解释，属于"注释与事实不完全对齐"的情况，
判断时以表的实际内容为准）。

**③ "Numbers 548 and above are available"**
548 以上才是干净的、不带历史包袱的新号码空间。

### 2.3 ⭐ 为什么 x32 要从 512 开始？——一个纯粹的 cache 决策

v5.1 的表里有一句更详细的注释（v6.6 版本已精简掉）：

```c
# x32-specific system call numbers start at 512 to avoid cache impact
# for native 64-bit operation. The __x32_compat_sys stubs are created
# on-the-fly for compat_sys_*() compatibility system calls if X86_X32
```

⭐⭐ **"to avoid cache impact for native 64-bit operation"**

系统调用表在内核里是一个**按号码索引的数组**。如果 x32 的号码和 64 位的号码
交错排布，表的有效跨度就会变大，原生 64 位进程访问表时的 **cache 局部性会变差**。
把 x32 整体推到 512 以上，原生路径就只触碰 0–334 这一小段热区。

这是 Unix「简单原语」哲学的延续——**连号码怎么分配都要算 cache**。
335–423 那段就是当年"考虑过但最终放弃"的中间方案留下的空档。

### 2.4 版本断崖：跳跃发生在 v5.1

用「最大号码」追踪：

| tag | 最大号码 | 64 位条目数 | 说明 |
|-----|---------|------------|------|
| v4.9 | 331 | 332 | |
| **v5.0** | **334** | 335 | 最后一个是 `rseq` |
| **v5.1** | **427** | 339 | ⭐ **跳过 335–423，从 424 起** |
| v5.10 | 440 | 352 | |
| v6.0 | 450 | 362 | |
| v6.6 | 453 | 365 | |

⭐ **v5.0 → v5.1 是一次性跳过去的。** 而 v5.1 新增的那批恰好是：

```
424	common	pidfd_send_signal
425	common	io_uring_setup      ⭐
426	common	io_uring_enter      ⭐
427	common	io_uring_register   ⭐
```

**io_uring 是历史上第一批拿到 424+ 号码的系统调用。**

这个巧合很有意味：io_uring 是「对 Unix 抽象税的反叛」（见 §6），
而它在号码表上的位置正好标志着**新旧时代的分界线**——
334 及以前是"Unix 遗产"那一侧，424 及以后是"后 io_uring"那一侧。

> ⚠️ 注意读法：v5.1 的"最大号码"从 334 跳到 427 **不代表新增了 93 个 syscall**。
> 64 位条目数只从 335 涨到 339（+4，就是上面那四个）。
> **号码会跳，条目数才是真实数量**——统计 syscall 数量时一定看条目数。

---

## §3 ⭐ 「少而稳」的量化：每年新增约 5 个

把各版本的 64 位条目数排成曲线：

| tag | 发布年份 | 64 位 syscall 数 |
|-----|---------|-----------------|
| v4.9 | 2016 | 332 |
| v5.0 | 2019 | 335 |
| v5.1 | 2019 | 339 |
| v5.10 | 2020 | 352 |
| v6.0 | 2022 | 362 |
| v6.6 | 2023 | **365** |

**v4.9 → v6.6（约 7 年）：332 → 365，净增 33 个，平均每年约 5 个。**

⭐ 这就是「接口少而稳」的硬证据。对比一下其它软件层的接口膨胀速度，
内核 syscall 表的增长慢得不可思议——**因为每加一个 syscall 都是永久承诺**
（用户空间一旦依赖，就再也不能改语义或删掉）。

**这也解释了为什么新特性越来越倾向于"不加 syscall"**：
- `io_uring`：3 个 syscall 撑起上百种操作（而不是加 100 个 syscall）
- `eBPF`：1 个 `bpf()` syscall 承载整个子系统
- `prctl` / `setsockopt` / `fcntl`：老牌"万能入口"，靠参数扩展而非新号

**HFT 关联**：这条对网关开发是**好消息**——
内核 ABI 稳定意味着多年积累的 syscall 层调优经验不会轻易作废；
同时也意味着**不要指望靠"等内核加新接口"来解决延迟问题**，
新接口来得很慢，且往往先服务于通用场景而非极致延迟。

---

## §4 x32 ABI：一个正在退场的历史角色

§2 提到的那个"historical design error"，主角就是 **x32 ABI**。

| | 含义 |
|---|---|
| 是什么 | x86_64 的 **ILP32** 变体：跑 64 位指令，但 `long`/指针是 32 位 |
| 图什么 | 省掉 64 位指针的内存开销（缓存占用、内存带宽） |
| 代价 | 号码空间要单独划一段（512–547），且有一批 syscall 号码与原生不同 |
| 现状 | v6.6 仍保留 36 个 x32 专属条目，但**注释明确写着"Do not add new syscalls to this range"** |

从 `syscall_64.tbl` 的三类 ABI 计数也能看出它的边缘地位：

```
     48 64          ← 仅 64 位可用
    317 common      ← 共用（主力）
     36 x32         ← 仅 x32 可用（不到十分之一）
```

**教训**：x32 是"用复杂度换内存"的尝试，在指针普遍 64 位、
内存不再那么贵的今天，这笔交易不再划算，于是它成了号码表上
一块**被冻结的化石区**——留着兼容，但不再生长。

---

## §5 现代演进的三个时间节点（v6.6 视角的定位）

原笔记的「五条优势」表里有两处现代演化结论，这里给出版本实证。

### 5.1 Rust 进内核：**v6.1**

二分证据（`rust/Makefile` 的字节数）：

| tag | `rust/Makefile` | 结论 |
|-----|-----------------|------|
| v5.19 | 66 B（404 残片） | 不存在 |
| **v6.0** | **66 B（404 残片）** | **不存在** |
| **v6.1** | **16256 B** | ⭐ **引入** |

所以原笔记写的「+少量 Rust，**6.1+**」是准确的。
但要注意 v6.6 时 Rust 的**实际覆盖面**：主要用于**驱动和部分子系统**，
核心路径（调度器、内存管理、网络栈）仍全是 C。
「用 Rust 重写内核」在可见的未来不会发生——它是**增量补充**不是替代。

### 5.2 ⭐ PREEMPT_RT 合入主线：**v6.12**

这是比 Rust 更重要的一个节点，尤其对 HFT。
判据用 `arch/x86/Kconfig` 里的 `select ARCH_SUPPORTS_RT`：

| tag | `ARCH_SUPPORTS_RT`（x86） | 含义 |
|-----|---------------------------|------|
| v6.6 | **0** | 需要打 RT 补丁 |
| v6.11 | **0** | 仍需补丁 |
| **v6.12** | **1** | ⭐ **RT 进入主线** |
| v6.13 | 1 | |
| v6.14 | 1 | |

⭐ **v6.12 之前，要跑真正的实时内核必须打 out-of-tree 的 PREEMPT_RT 补丁；
v6.12 起，主线内核原生支持。**

这直接改变了 Ch10 全部选型结论的适用范围——Ch10.9 讲过
`spinlock_t` 在 RT 上会变成睡眠锁，而以前这需要额外打补丁才能遇到，
现在**升级内核就可能遇到**。

### 5.3 ⭐ PREEMPT_LAZY：**v6.13** 新增的第五个抢占模型

Ch10.9 讲过四个抢占模型（`PREEMPT_NONE` / `VOLUNTARY` / `PREEMPT` / `PREEMPT_RT`）。
**v6.13 加了第五个**：

| tag | `ARCH_HAS_PREEMPT_LAZY`（x86） |
|-----|-------------------------------|
| v6.12 | 0 |
| **v6.13** | **1** ⭐ 引入 |
| v6.14 | 1 |

`kernel/Kconfig.preempt`（v6.13）里的定义：

```
config PREEMPT_LAZY
	bool "Scheduler controlled preemption model"
	depends on !ARCH_NO_PREEMPT
	depends on ARCH_HAS_PREEMPT_LAZY
	select PREEMPT_BUILD if !PREEMPT_DYNAMIC
	help
	  This option provides a scheduler driven preemption model that
	  is fundamentally similar to full preemption, but is less
	  eager to preempt SCHED_NORMAL tasks in an attempt to
	  reduce lock holder preemption and recover some of the performance
	  gains seen from using Voluntary preemption.
```

拆成三句：

1. **"fundamentally similar to full preemption"** —— 本质上接近 `PREEMPT`（完全抢占）
2. **"but is less eager to preempt SCHED_NORMAL tasks"** —— 但不那么急于抢占**普通任务**
3. 目的：**"to reduce lock holder preemption"** —— 减少**持锁者被抢占**

⭐⭐ 第 3 句是关键，它直指 Ch10 反复出现的那个痛点：
**持锁者被抢占 = 优先级反转的根源**。
- Ch10.5：mutex 有乐观自旋（OSQ），就是为了让等待者"等一等"而不是立刻睡，
  赌持锁者马上放锁
- Ch10.8：seqlock 在 RT 上写者会被低优先级读者饿死——同一个问题的另一面
- Ch10.11 §3.6：rwlock 在 RT 上的写者饥饿

`PREEMPT_LAZY` 的思路是：**对 `SCHED_NORMAL` 任务"懒一点"再抢占**，
让持锁者有机会跑完临界区，同时保留对 RT 任务的立即抢占。
这是对"完全抢占导致大量持锁抢占"这个副作用的一次定向修补。

**HFT 关联**：如果你的负载是「少数 RT 任务 + 大量普通任务」，
`PREEMPT_LAZY` 可能是比 `PREEMPT` 或 `PREEMPT_RT` 更合适的第三选项——
它既保住了 RT 任务的低延迟，又减少了普通任务持锁被打断造成的抖动。

### 5.4 v6.6 的时代定位

| 事项 | v6.6（本笔记基准）的状态 |
|------|-------------------------|
| Rust | ✅ 已引入（v6.1），覆盖驱动层 |
| PREEMPT_RT | ❌ **尚未合入主线**（v6.12 才合） |
| PREEMPT_LAZY | ❌ 不存在（v6.13 才有） |
| 内核代号 | `NAME = Hurr durr I'ma ninja sloth` |

（v6.6 的代号出自 `Makefile`，Linus 一贯的无厘头命名传统，
每个稳定版都有一个——这也是 Unix 文化的一部分。）

---

## §6 「Unix 抽象税」与三条逃税路线（syscall 视角）

原笔记说「先懂税再谈逃税」。从 §1–§3 的数据看，这笔税具体是什么？

### 6.1 税在哪

一次 `read()` 要经过：

```
用户态 ──syscall 指令──► entry_SYSCALL_64
                          ├─ 切到内核栈、保存寄存器
                          ├─ 从 syscall 表按号查函数      ← §2 那张表
                          ├─ 参数检查、fd → struct file
                          ├─ VFS：file_operations.read
                          ├─ 具体文件系统 / socket 层
                          ├─ 拷贝到用户缓冲区             ← 最容易忽略的一笔
                          └─ 返回用户态、恢复寄存器
```

其中**两笔最贵的税**：
1. **模式切换本身**（syscall / return，各约几十到上百周期，含 TLB/缓存污染）
2. **数据拷贝**（内核缓冲区 → 用户缓冲区）

### 6.2 三条逃税路线

| 路线 | 绕掉什么 | 代价 |
|------|---------|------|
| **`mmap`** | 拷贝（共享页，不 copy） | 仍然要走 VFS；缺页异常是新的延迟源 |
| **`io_uring`** | **模式切换的批量化**（一次 syscall 提交/收割多笔 IO） | 仍是 fd 抽象；需要新学一套编程模型 |
| **DPDK / AF_XDP** | **整个内核网络栈 + VFS**（用户态直接轮询网卡） | 独占网卡（或队列）；失去内核所有协议栈功能 |

⭐ 注意 `io_uring` 的位置很特殊：它**没有绕开 Unix 抽象**（还是 fd、还是 syscall），
它只是把**每次操作的固定开销摊薄**了。而 DPDK/AF_XDP 是**真的绕开**——
用「放弃内核协议栈」换「零拷贝 + 零模式切换 + 轮询代替中断」。

**判据**：
- 能接受 fd 抽象、只是嫌 syscall 太频繁 → **io_uring**
- 连内核协议栈都嫌慢、且网卡可以独占 → **AF_XDP** / **DPDK**

### 6.3 一个容易搞反的点

⚠️ **io_uring 不是"更快的 syscall"，是"更少的 syscall"。**
单次操作的延迟它未必比 `read()` 低（甚至可能略高，因为多了队列操作），
它的收益来自**批量化后的摊薄**——
把 N 次 syscall 的固定开销变成约 1 次。

所以评估 io_uring 收益时，看的是**吞吐和尾延迟的分布**，
不是单次操作的最小时延。

---

## §7 HFT  Checklist：从 Unix 遗产里该拿什么、该躲什么

| 遗产 | 拿 | 躲 |
|------|-----|-----|
| 少 syscall | 热路径只用十几个 syscall，审计面小 | 冷路径也无所谓，别过度优化 |
| 一切皆文件 | `epoll` 统一管理 fd 是成熟方案 | ⚠️ 每个 fd 都是一次 VFS 跳转；万级 fd 时 epoll 本身成为瓶颈 |
| `fork` + COW | 进程隔离好，崩溃不扩散 | 热路径上 fork 是禁区（页表复制 + TLB 刷新） |
| C 实现 | 与内核同语言，无 FFI 开销 | 内存安全全靠自己（Rust 目前救不了核心路径） |
| POSIX | 代码可移植，换发行版成本低 | ⚠️ POSIX 保证的是**语义**不是**延迟**，别指望它保证实时性 |

### 版本选择的实际建议

| 场景 | 建议 |
|------|------|
| 要真实时、能接受新内核 | **≥ v6.12**（RT 已进主线，不用打补丁） |
| 要「RT 任务 + 普通任务」混合负载 | **≥ v6.13**（可试 `PREEMPT_LAZY`） |
| 保守生产环境 | v6.6 这类 LTS + 官方 RT 补丁（如果确实需要 RT） |
| 只看 syscall 表 | 各版本差异不大（每年 +5），不是选型依据 |

---

**HFT 对照：** 网关仍活在 **「少 syscall、少拷贝、快 fork/线程」** 的 Unix 遗产里 — 热路径 **`read`/`send`/`mmap`** 皆是「一切皆文件」后代；而绕开它的两条路（DPDK 用户态轮询、io_uring 直投队列）本质都是**对 Unix 抽象税的 revolt**——先懂税再谈逃税。

**本篇补充的 HFT 视角**：
- ⭐ **syscall 表的 89 个空洞**（§2）说明：内核连号码分配都在算 cache。
  这不是"历史八卦"，是**内核设计的一贯取向**——你在自己代码里做的每一个
  数据结构布局决策，都应该有同样的自觉。
- ⭐ **RT 进主线（v6.12）** 意味着 Ch10 全部「RT 上会怎样」的推理
  从"特殊情况"变成了"默认可能"。**升级内核时要重测。**
- ⭐ **PREEMPT_LAZY（v6.13）** 是「混合负载」场景的新选项，
  它针对的正是 Ch10 反复讨论的**持锁者被抢占**问题。

→ [03-linux-userspace-api](../../../03-linux-userspace-api/) · [02-CSAPP Ch8](../../../02-computer-systems/chapter-08-exceptional-control-flow/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Unix 只有几百个系统调用，这为什么对 HFT 有利？

<details><summary>答案</summary>

系统调用少意味着 syscall 入口审计面小、热路径可预测。HFT 网关热路径只用 read/write/mmap/send 等十几个 syscall，减少调度器与 VFS 层分支。少而稳的接口 = 内核版本升级时 ABI 兼容性好。

</details>

<details><summary>按 v6.6 修订/补充</summary>

**结论对，但「几百个」可以精确化（§1）。** v6.6 的 x86_64 上：

| 指标 | 数值 |
|------|------|
| 表内条目总数 | 401（含 x32） |
| **64 位实际可用** | **365** |
| 号码空间 | 0–453，中间有 **89 个洞**（335–423） |
| `__NR_syscalls` | 453（`unistd.h:827`） |

**这个数量级的意义要靠对比才看得出来**：内核对用户的承诺面（365 个 syscall）
比它之上的任何一层都小一个数量级——glibc 导出 2000+ 个符号，
一个桌面应用的依赖树动辄数万个符号。

**而「稳」这一点也有硬数据（§3）**：

| tag | 年份 | 64 位 syscall 数 |
|-----|------|-----------------|
| v4.9 | 2016 | 332 |
| v6.6 | 2023 | 365 |

**7 年净增 33 个，平均每年约 5 个。** 内核 syscall 表的膨胀慢得不可思议，
因为**每加一个 syscall 都是永久承诺**（用户空间一旦依赖就再也不能改语义）。

这也导致了一个重要的设计趋势：**新特性越来越倾向于不加 syscall**——
- `io_uring`：3 个 syscall 撑起上百种操作
- `eBPF`：1 个 `bpf()` 承载整个子系统
- `prctl` / `setsockopt`：靠参数扩展而非新增号码

**HFT 推论**：内核 ABI 稳定 → 你多年积累的 syscall 层调优经验不会轻易作废；
但也 → **不要指望"等内核加新接口"来解决延迟问题**，新接口来得很慢，
且往往先服务通用场景而非极致延迟。

</details>

**Q2.** Unix「一切皆文件」设计哲学对现代网络编程有什么局限？

<details><summary>答案</summary>

一切皆文件让 socket/fd 统一接口，但网络包仍需从内核拷贝到用户态（除非用 zero-copy）。HFT 用 AF_XDP/DPDK 绕过 VFS 直接从网卡取包，就是因为「文件抽象」对纳秒级延迟仍有开销。

</details>

<details><summary>按 v6.6 修订/补充</summary>

**结论对，但「拷贝」只是两笔税里的一笔（§6.1）。**
一次 `read()` 的完整成本可以拆成两大块：

| 税种 | 内容 | 大致位置 |
|------|------|---------|
| **模式切换** | syscall + return，切栈/存寄存器/TLB 与缓存污染 | 每次 syscall 固定付 |
| **数据拷贝** | 内核缓冲区 → 用户缓冲区 | 按数据量付 |

原答案只提到了第二笔，**第一笔（模式切换）在高频小包场景下往往才是大头**——
因为小包的拷贝量很小，但模式切换的开销是固定的。

**因此三条逃税路线要按"绕掉了什么"来区分（§6.2）**：

| 路线 | 绕掉 | 代价 |
|------|------|------|
| `mmap` | **拷贝** | 仍走 VFS；缺页异常是新延迟源 |
| `io_uring` | **模式切换的批量化** | 仍是 fd 抽象；要学新编程模型 |
| DPDK / AF_XDP | **整个内核网络栈 + VFS** | 独占网卡/队列；失去协议栈 |

⭐ **一个容易搞反的点（§6.3）**：
**`io_uring` 不是"更快的 syscall"，是"更少的 syscall"。**
单次操作延迟它未必比 `read()` 低（甚至略高，多了队列操作），
收益来自**批量化后的摊薄**——把 N 次 syscall 的固定开销变成约 1 次。

所以评估 io_uring 时应该看**吞吐和尾延迟分布**，而不是单次操作的最小时延。
这个误判很常见：拿单次 `read()` 和单次 io_uring 提交比，发现后者更慢，
就断定 io_uring 没用——**比错了维度**。

**判据**：能接受 fd 抽象、只是嫌 syscall 太频繁 → io_uring；
连内核协议栈都嫌慢且网卡可独占 → AF_XDP / DPDK。

</details>

**Q3.** fork() 的「独特」体现在哪？它的性能问题后来是怎么被逐步修补的？

<details><summary>答案</summary>

独特在**复制整个地址空间的语义**（子进程获得父进程的完整副本）——比 spawn（传可执行路径重新加载）语义强大得多：可以在 fork 后、exec 前对副本做任意操作（重定向、改环境）。修补史：① **vfork**（BSD）——不复制，父挂起等 exec，过渡方案；② **COW**（现代 fork）——只复制页表不复制页，写时才拷——fork 成本从 O(内存) 降到 O(页表)；③ **posix_spawn**——COW 时代仍有场景省 fork（单页进程池）；④ **线程**（clone 共享地址空间）——彻底绕开复制语义。链条读法：语义的优雅先用，性能的账慢慢还。

</details>

<details><summary>按 v6.6 修订/补充</summary>

这条修补链条讲得很完整，补一个**量级感和一条 HFT 禁区**：

**① COW 到底把成本降到了哪里**
- 无 COW：`fork` 要复制**整个地址空间**的内存页 → O(内存量)
- 有 COW：只复制**页表** → O(页表项数)

对一个占用 1 GB 的进程，页表可能只有几 MB——**降了约两个数量级**。
但注意它**不是免费的**：页表本身要分配和拷贝，
而且之后父子进程每次写页都会触发**缺页异常 + 实际拷贝**（写时拷贝的"写"）。

**② HFT 禁区：热路径上不要 fork**
即便有 COW，`fork` 仍然会：
- 分配并复制页表（GB 级进程是 MB 级拷贝）
- **刷新/污染 TLB**
- 走一遍调度器（新 task 入队）
- 如果之后真的写内存，还会有大量 COW 缺页

→ 这些全是**不可预测的延迟源**。热路径上要用**预先创建好的进程/线程池**，
或者干脆用线程（`clone` 共享地址空间，连页表都不复制）。

**③ "语义的优雅先用，性能的账慢慢还"这条读法可以推广**
它不只是 fork 的故事，是**整个 Unix 设计史的重复模式**：
- 「一切皆文件」→ 优雅，后来用 io_uring / AF_XDP 还账
- 「进程即资源容器」→ 优雅，后来用线程 / `posix_spawn` 还账
- `x32` ABI → 想省内存（优雅的目标），结果**号码表上留下 89 个洞**（§2），
  成了内核自己承认的 "historical design error"

⭐ 第三条的教训最直接：**为性能做的妥协，往往会在别处留下永久疤痕。**
x32 当年省下的那点指针内存，代价是号码表上永远冻结的 335–423 和 512–547 两段。

</details>

**Q4.** v6.6 的 x86_64 上到底有多少个系统调用？号码 335–423 为什么是空的？

<details><summary>答案</summary>

**数量（数 `arch/x86/entry/syscalls/syscall_64.tbl` 得来）**：

| 指标 | 数值 |
|------|------|
| 表内条目总数 | **401**（含 x32） |
| 64 位可用 | **365**（`common` 317 + `64` 专属 48） |
| x32 专属 | 36（号码 512–547） |
| 最大号码 | 547 |
| 空洞 | **89 个，全在 335–423 这一段** |
| 未实现存根 | 1 个：`156 _sysctl` → `sys_ni_syscall` |

⚠️ **统计 syscall 数量要看"条目数"不是"最大号码"**——号码会跳，
v5.1 时最大号从 334 直接跳到 427，但条目只多了 4 个。

**空洞的原因（表内注释原文，v6.6）**：

```c
# don't use numbers 387 through 423, add new calls after the last
# 'common' entry
#
# Due to a historical design error, certain syscalls are numbered differently
# in x32 as compared to native x86_64.  These syscalls have numbers 512-547.
# Do not add new syscalls to this range.  Numbers 548 and above are available
# for non-x32 use.
```

而 v5.1 的表里有一句更详细的说明：

```c
# x32-specific system call numbers start at 512 to avoid cache impact
# for native 64-bit operation.
```

⭐⭐ **根因是 cache**：系统调用表是按号码索引的数组。
如果 x32 和 64 位号码交错，表的有效跨度变大，
原生 64 位路径访问表时的 **cache 局部性变差**。
所以把 x32 整体推到 512+，让原生路径只触碰 0–334 这一小段热区。

**版本断崖**：跳跃发生在 **v5.1**——

| tag | 最大号码 | 64 位条目 |
|-----|---------|----------|
| v5.0 | 334（`rseq`） | 335 |
| **v5.1** | **427** | 339 |

而 v5.1 新增的四个恰好是号码 424–427：

```
424	common	pidfd_send_signal
425	common	io_uring_setup
426	common	io_uring_enter
427	common	io_uring_register
```

**io_uring 是历史上第一批拿到 424+ 号码的 syscall。**

</details>

**Q5.** PREEMPT_RT 和 PREEMPT_LAZY 分别是在哪个版本进入主线 x86 的？它们是什么关系？

<details><summary>答案</summary>

**PREEMPT_RT：v6.12 合入主线（x86）**

判据是 `arch/x86/Kconfig` 里的 `select ARCH_SUPPORTS_RT`：

| tag | x86 的 `ARCH_SUPPORTS_RT` |
|-----|---------------------------|
| v6.6 | **0**（要打 out-of-tree RT 补丁） |
| v6.11 | **0** |
| **v6.12** | **1** ⭐ 合入主线 |
| v6.13 / v6.14 | 1 |

⭐ 意义：**v6.12 之前要跑真正的实时内核必须打补丁；v6.12 起主线原生支持。**
这让 Ch10 全部「RT 上会怎样」的推理（spinlock 变睡眠锁、
rwlock 写者饥饿反转、local_lock 变 per-CPU spinlock）
从"特殊情况"变成"默认可能"——**升级内核时要重测**。

**PREEMPT_LAZY：v6.13 新增（第五个抢占模型）**

| tag | x86 的 `ARCH_HAS_PREEMPT_LAZY` |
|-----|-------------------------------|
| v6.12 | 0 |
| **v6.13** | **1** ⭐ 引入 |
| v6.14 | 1 |

Ch10.9 讲过四个抢占模型（`NONE` / `VOLUNTARY` / `PREEMPT` / `RT`），
v6.13 加了第五个。`kernel/Kconfig.preempt`（v6.13）的定义：

```
config PREEMPT_LAZY
	bool "Scheduler controlled preemption model"
	depends on ARCH_HAS_PREEMPT_LAZY
	help
	  This option provides a scheduler driven preemption model that
	  is fundamentally similar to full preemption, but is less
	  eager to preempt SCHED_NORMAL tasks in an attempt to
	  reduce lock holder preemption and recover some of the performance
	  gains seen from using Voluntary preemption.
```

三句话拆开：
1. 本质上接近 `PREEMPT`（完全抢占）
2. 但**不那么急于抢占 `SCHED_NORMAL` 任务**
3. 目的是 **"reduce lock holder preemption"**——减少持锁者被抢占

**两者的关系**：不是替代关系，是**互补的两个维度**。
- `PREEMPT_RT` 解决的是"**能不能**抢占"（把更多内核路径变成可抢占的）
- `PREEMPT_LAZY` 解决的是"**该不该这么急着**抢占"（对普通任务懒一点，
  让持锁者有机会跑完临界区，同时保留对 RT 任务的立即抢占）

⭐ 第 3 句直指 Ch10 反复出现的痛点：**持锁者被抢占 = 优先级反转的根源**。
Ch10.5 的 mutex 乐观自旋（OSQ）、Ch10.8 的 seqlock RT 活锁、
Ch10.11 §3.6 的 rwlock RT 写者饥饿，全是同一个问题的不同侧面。

**HFT 关联**：负载是「少数 RT 任务 + 大量普通任务」时，
`PREEMPT_LAZY`（≥ v6.13）可能是比 `PREEMPT` 或 `PREEMPT_RT` 更合适的第三选项——
既保住 RT 任务的低延迟，又减少普通任务持锁被打断造成的抖动。

</details>

**Q6.** 从「历史化石」的角度，举一个 v6.6 源码里能看到的「当年的妥协留下的永久疤痕」。

<details><summary>答案</summary>

**首选答案：x32 ABI 在系统调用号码表上留下的两段冻结区（本篇 §2、§4）。**

疤痕长这样：

```
  0 ──────────── 334    335 ────────── 423    424 ─── 453    512 ─── 547
  ┌──────────────────┐  ┌──────────────────┐  ┌────────────┐  ┌──────────┐
  │ 原生 64 位在用    │  │ 89 个空号（禁区）  │  │ 后用区      │  │ x32 专属  │
  └──────────────────┘  └──────────────────┘  └────────────┘  └──────────┘
```

三条证据：

1. **内核自己承认错误**（v6.6 表内注释原文）：
   > **"Due to a historical design error"**, certain syscalls are numbered
   > differently in x32 as compared to native x86_64.

2. **退场声明**：
   > Do not add new syscalls to this range. **Numbers 548 and above are available**.

3. **x32 的边缘地位**（三类 ABI 计数）：
   ```
        48 64
       317 common      ← 主力
        36 x32         ← 不到十分之一，且不再生长
   ```

**完整因果链**：
x32 想用 ILP32（64 位指令 + 32 位指针）省内存带宽 →
需要单独的 syscall 号码 → 为了**不污染原生 64 位的 cache 局部性**
把它们推到 512+（v5.1 注释：`to avoid cache impact for native 64-bit operation`）
→ 中间 335–423 成了"考虑过但放弃"的空档 → 号码表上永远留下 89 个洞。

**教训（可以推广的一条）**：
⭐ **为性能做的妥协，往往会在别处留下永久疤痕。**
x32 当年省下的那点指针内存，代价是号码表上永远冻结的两段区间，
外加一句"这是一个历史设计错误"的注释。这正是 Unix 那条
「语义的优雅先用，性能的账慢慢还」的镜像版本——
这次是**性能的优化先用，语义的债慢慢还**。

**其它可选答案**（同样成立）：
- **`156 _sysctl` → `sys_ni_syscall`**：整个表里唯一的"未实现存根"。
  `sysctl()` 系统调用被 `/proc/sys` 取代后，号码**永远不能回收**
  （用户空间可能有代码在调），只能留一个返回 `-ENOSYS` 的桩。
- **`sys_open` 还在，但新代码都用 `openat`**：`open` 的 TOCTOU 问题
  无法在保持语义的前提下修复，于是加了一族 `*at()` 而不是改 `open`。

</details>

</details>
---

### 本篇与其它章节的交叉引用

| 本篇主题 | 交叉点 | 详见 |
|---------|--------|------|
| syscall 表的 cache 考量 | 数据结构布局的 cache 自觉 | — |
| io_uring 批量化 | 与「少 syscall」哲学的关系 | [Ch13 VFS](../../chapter-13-vfs/) |
| syscall 进入内核的路径 | 本章 §6.1 的调用链 | [Ch5 系统调用](../../chapter-05-system-calls/) |
| PREEMPT_RT / LAZY | ⭐ 与 Ch10 全部锁语义相关 | [Ch10.9 禁止抢占](../../chapter-10-sync-methods/notes/section-10.9-禁止抢占.md) |
| 持锁者被抢占 | ⭐ 优先级反转的根源 | [Ch10.5 互斥体](../../chapter-10-sync-methods/notes/section-10.5-互斥体.md) |
| RT 上的 seqlock / rwlock | RT 进主线后的适用范围 | [Ch10.11 选型速查](../../chapter-10-sync-methods/notes/section-10.11-选型速查Ch-9--Ch-10.md) |

---
