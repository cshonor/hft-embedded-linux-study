# Ch 15 BPF 技术 · BPF

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**第二版最重磅新增** — **eBPF** 把 Linux 观测从「固定工具」推进到 **可编程内核态分析**。Ch 4 把 BPF 列为工具链之一；Ch 5–14 各章反复出现的 `runqlat`、`biolatency`、`tcplife` 等 **均出自 BCC**；本章是 **BCC + bpftrace 总入口**。  
> **HFT：** 生产裸机 **bpftrace/BCC 与 perf 并列标配** — off-CPU、run queue、重传、direct reclaim；深度专书 → [03-BPF-Performance-Tools](../03-BPF-Performance-Tools/)。

---

## 大白话 · 本章就五件事

> **eBPF = 内核里的安全小程序 + 两种输出：明细 ring buffer / 聚合 maps。**

**① 从抓包过滤器到通用内核 VM — 1992 BPF → 2013+ eBPF。**

- 程序跑在内核，**Verifier** 保证不崩内核 — 加载失败 = 改程序，别 `--force`。

**② BCC — 成套预制工具 + Python/Lua 开发框架。**

- `execsnoop`、`runqlat`、`tcplife`… — **日常直接跑**；复杂工具用 BCC 打包发布。

**③ bpftrace — 类 awk 语言，单行命令之王。**

- 即兴追 kprobe/uprobe/USDT — **ad hoc** 根因分析；本仓库 [附录 C](./appendix-C-bpftrace单行命令.md) 扩展。

**④ BCC vs bpftrace：复杂工具 vs 快速脚本 — 双剑互补。**

- 不是二选一 — **先 BCC 标准工具，不够再 bpftrace 定制**。

**⑤ Maps 在内核聚合 — 高频率事件不打爆用户态。**

- 直方图、计数器在 kernel 汇总 — 与 Ftrace hist（Ch 14）同思路，更灵活。

下面按原书 15.1–15.2 及架构基础展开。

---

## BPF 背景与架构（15.1–15.2 基础）

### 演进

| 阶段 | 内容 |
|------|------|
| **经典 BPF（1992）** | Berkeley Packet Filter — tcpdump 加速包过滤 |
| **eBPF（2013+）** | 通用 **内核态 VM** — 追踪、网络(XDP)、安全、调度… |
| **第二版 SysPerf** | 全书工具链 **perf / Ftrace / BCC / bpftrace** 四支柱 |

### 安全：Verifier

```
用户编写 BPF 程序 → 加载到内核
    → Verifier 静态分析（边界、循环、指针）
    → 通过 → 附加到 hook（kprobe/tracepoint/XDP…）
    → 失败 → 拒绝加载（看 dmesg / bpftool）
```

| Verifier 保证 | 含义 |
|---------------|------|
| 无越界访问 | 不能乱读内核内存 |
| 有界循环 | 不能死循环拖死内核 |
| 类型安全 | 指针追踪 |

**HFT：** 生产只跑 **已知脚本**；自定义 bpftrace 先在 **staging** 验证加载。

### 数据输出：Ring Buffer vs Maps

| 机制 | 用途 | 开销 |
|------|------|------|
| **perf ring buffer** | **每事件** 明细（栈、timestamp、字段）→ 用户态 | 高事件率时大 |
| **BPF maps** | 内核 **聚合** — 计数、直方图、哈希 | 低 — 只读汇总 |

```
高频率 sched_switch：
  ❌ 每条送到用户态 → 打爆
  ✅ map histogram / BCC 内置聚合 → 只看分布
```

**Map 类型（常见）：**

| 类型 | 用途 |
|------|------|
| `HASH` / `ARRAY` | KV 计数 |
| `HISTOGRAM` | 延迟直方图（log2 桶） |
| `PERCPU_*` |  per-CPU 计数 — 减锁 |
| `STACK_TRACE` | 栈 ID 映射 |

→ Ch 14 [Ftrace hist](./chapter-14-Ftrace跟踪.md#1451471410-事件源filter-与-hist-triggers) 对比

### 挂载点（Hook）概览

| Hook | 说明 | 例子 |
|------|------|------|
| **tracepoint** | 稳定内核静态点 | syscalls、sched、block |
| **kprobe/kretprobe** | 内核函数动态 | `tcp_sendmsg` |
| **uprobe** | 用户函数 | strategy 内函数 |
| **USDT** | 用户静态探针 | 应用预埋 |
| **XDP / tc** | 网络最早/ qdisc | [03-BPF XDP note](../03-BPF-Performance-Tools/note-XDP与tc-BPF.md) |

---

## 15.1 BCC (BPF Compiler Collection)

### 是什么

| 组成 | 说明 |
|------|------|
| **bcc 库** | C/Python/Lua 写 BPF，编译加载 |
| **bcc-tools** | 大量 **预制单用途工具** — `/usr/share/bcc/tools/` |
| **libbpf 时代** | 新工具渐迁 **libbpf + CO-RE** — 03-BPF 专书详述 |

### 安装与危机清单

```bash
# Debian/Ubuntu 示例
sudo apt install bpfcc-tools linux-headers-$(uname -r)

# 验证
ls /usr/share/bcc/tools/ | head
sudo biolatency-bpfcc -h 2>/dev/null || sudo biolatency -h
```

→ Ch 4 [危机工具包](./chapter-04-观测工具.md#41-工具覆盖范围与危机工具)

### 工具地图（与前文章节对照）

| 领域 | BCC 工具 | SysPerf 章 |
|------|----------|------------|
| **CPU** | `profile`, `runqlat`, `runqlen`, `cpudist` | Ch 6 |
| **内存** | `drsnoop`, `wss` | Ch 7 |
| **文件/盘** | `opensnoop`, `filetop`, `cachestat`, `biolatency`, `biosnoop`, `biotop`, `biostacks` | Ch 8–9 |
| **网络** | `tcplife`, `tcptop`, `tcpretrans`, `tcpconnect`, `gethostlatency` | Ch 10 |
| **进程** | `execsnoop`, `execsnoop` | Ch 5 |
| **综合** | `hardirqs`, `softirqs`, `offcputime` | Ch 5–6 |

```bash
# 调度延迟分布（Ch 6 金标准）
sudo runqlat-bpfcc 10

# 块 I/O 延迟直方图（Ch 9）
sudo biolatency-bpfcc -F -m 5 10

# TCP 连接生命周期（Ch 10）
sudo tcplife-bpfcc

# Off-CPU 栈（Ch 5 — 与 perf 互补）
sudo offcputime-bpfcc -p $(pidof strategy) 30
```

### BCC 适用场景

| 适合 | 例子 |
|------|------|
| **标准工具日常化** | runbook 固定几条 BCC |
| **复杂多事件工具** | 需状态机、多 map 协作 |
| **打包给团队** | Python CLI 封装 |

**开发：** Python + BCC — 比 bpftrace 冗长，但 **可维护、可发布**。

→ [03-BPF ch04 BCC](../03-BPF-Performance-Tools/chapter-04-BCC.md)

---

## 15.2 bpftrace

### 是什么

**高级追踪语言** — 语法类似 awk/C，**单行命令** 极快。

```bash
# 统计 read syscall 调用次数
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = count(); }'

# 按进程统计 open 路径
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat /pid==$(pidof strategy)/ { @[comm] = count(); }'

# uprobe 用户函数延迟直方图
sudo bpftrace -e 'uprobe:/path/strategy:decode { @start[tid] = nsecs; }
    uretprobe:/path/strategy:decode /@start[tid]/ { @lat = hist(nsecs - @start[tid]); delete(@start[tid]); }'
```

### 单行命令优势

| 场景 | bpftrace |
|------|----------|
| **即兴假设验证** | 「是不是这个内核函数慢？」— 一行 kprobe |
| **定制 filter** | pid、comm、栈、直方图 |
| **USDT** | `usdt:...` 探针 |
| **教学/探索** | 比写 BCC Python 快 10× |

**本仓库：** [附录 C bpftrace 单行命令](./appendix-C-bpftrace单行命令.md) — SysPerf 配套备忘。

### bpftrace 适用场景

| 适合 | 不适合 |
|------|--------|
| Ad hoc 根因、一次性调查 | 需复杂 GUI、长期产品化 |
| 快速 kprobe/uprobe 实验 | 极老内核无 bpftrace |
| 与 BCC 工具 **组合** | 替代所有 BCC（不必） |

→ [03-BPF ch05 bpftrace](../03-BPF-Performance-Tools/chapter-05-bpftrace.md)

---

## 15.1.7 BCC vs bpftrace

| 维度 | **BCC** | **bpftrace** |
|------|---------|--------------|
| **语言** | Python/Lua + C BPF | 专用 DSL |
| **上手** | 跑预制工具快；开发慢 | 单行极快；复杂脚本中等 |
| **输出** | 成熟 CLI 格式 | 自定义 print/map |
| **维护** | 适合 **团队标准工具** | 适合 **个人诊断脚本** |
| **性能** | 优化充分 | 多数场景足够 |
| **关系** | **互补双剑** | **互补双剑** |

**Gregg 工作流：**

```
1. 生产 crisis → BCC 标准工具（runqlat、tcpretrans、biolatency…）
2. 标准工具不够 → bpftrace 即兴追 kprobe/uprobe
3. 证明重复有用 → 升格为 BCC 工具或 runbook 脚本
4. 长期产品 → 03-BPF 专书 + libbpf/CO-RE
```

**HFT runbook 示例：**

```
延迟尖刺
  → offcputime / runqlat（BCC）
  → 若 Lock → bpftrace 追 mutex
  → 若 Net → tcpretrans + ss -tiepm
  → 若 mystery stall → Ftrace hwlat（Ch 14）
```

---

## 与 perf / Ftrace 的分工（全书闭环）

| 工具 | 强项 |
|------|------|
| **perf** | CPU 采样、PMC、官方标配（Ch 13） |
| **Ftrace** | function_graph、hwlat（Ch 14） |
| **BCC** | 预制全栈工具、off-CPU、I/O/TCP 直方图 |
| **bpftrace** | 快速定制、单行 ad hoc |

```
Ch 1  60 秒清单
Ch 2  USE / 延迟分解
Ch 4  四工具链
Ch 5–10  各资源「用哪条 BCC」
Ch 13  perf
Ch 14  Ftrace
Ch 15  BPF（本章）
  → 03-BPF 专书 18 章
  → 附录 C 单行命令
```

---

## 本章 Checklist

- [ ] 裸机安装 **bcc-tools + bpftrace**，内核头匹配
- [ ] 理解 **Verifier、maps、ring buffer**
- [ ] 跑通 **runqlat、biolatency、tcpretrans** 各一次
- [ ] 写一条 **bpftrace** 统计 syscall 或 tracepoint
- [ ] 知道 **BCC 标准工具 vs bpftrace 定制** 何时切换
- [ ] 生产追踪 **限 PID、限时长** — 观测者效应（Ch 4）

---

## HFT 精读捷径（Ch 15 在路线中的位置）

```
SysPerf Ch 1–14  →  Ch 15 BPF 总入口
  → 03-BPF-Performance-Tools 全书（紧接 02 SysPerf）
  → 10-DPDK 02-Advanced XDP
  → 11-HFT 生产 runbook
```

**本章最小行动集：**

1. `sudo runqlat-bpfcc 10` — dedicated 核 run queue 延迟 baseline。
2. `sudo tcpretrans-bpfcc 30` — 发单通道有无重传。
3. `sudo bpftrace -e 'tracepoint:syscalls:sys_enter_{read,write} /pid==X/ { @[probe] = count(); }'` — 热路径 syscall 一览。
4. 将三条写入 **危机 runbook**（Ch 4）。

**Gregg 本章金句（HFT 版）：**

> **eBPF 是 Linux 观测的革命** — BCC 是 **标准武器库**，bpftrace 是 **现场即兴手术刀**。  
> **Maps 在内核聚合** — 高频率事件别往用户态灌水；**Verifier 通过** 才能上生产。

---

## 相关章节

- 上一章：[chapter-14-Ftrace跟踪.md](./chapter-14-Ftrace跟踪.md)
- 下一章：[chapter-16-案例研究.md](./chapter-16-案例研究.md)
- 工具地图：[chapter-04-观测工具.md](./chapter-04-观测工具.md)
- perf：[chapter-13-perf性能分析.md](./chapter-13-perf性能分析.md)
- 应用 Off-CPU：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- 附录 C：[appendix-C-bpftrace单行命令.md](./appendix-C-bpftrace单行命令.md)
- BPF 专书：[03-BPF-Performance-Tools](../03-BPF-Performance-Tools/)
- XDP：[03-BPF note-XDP](../03-BPF-Performance-Tools/note-XDP与tc-BPF.md) · [10-DPDK 02-Advanced](../10-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-XDP与DPDK对照.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
