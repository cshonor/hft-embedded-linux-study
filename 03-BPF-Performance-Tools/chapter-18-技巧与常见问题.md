# Ch 18 技巧与常见问题 · Tips, Tricks, and Common Problems

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**（生产用 BPF **强烈建议精读**）

> 本章定位：**全书压轴心法** — 无新工具，总结 **开销模型、采样技巧、排障方法论、六大常见坑**。Brendan Gregg 多年性能工程经验的浓缩。  
> **HFT：** 在 tick 核上跑 BPF 前 **必读** — **频率 × 动作 / CPU 数**、**99Hz 采样**、**帧指针/符号**、**勿 trace 追踪器自身**。与 [Ch 2](./chapter-02-技术背景.md)、[Ch 12](./chapter-12-语言.md)、[Ch 13 § libc 帧指针](./chapter-13-应用程序.md) 闭环。  
> **上一章：** [chapter-17-其他BPF工具.md](./chapter-17-其他BPF工具.md) · **附录：** [appendix-A-bpftrace单行命令.md](./appendix-A-bpftrace单行命令.md)

---

## 1. 本章在全书中的位置

```
Ch 1–5   技术与语言
Ch 6–16  工具按资源/环境
Ch 17    平台生态
Ch 18    ← 怎么用对、怎么不踩坑（无新工具）
附录 A–E 速查与开发
```

**没有新工具** — 但有 **决定是否该用、用哪种、为何失败** 的判断力。

---

## 第一部分：提示与技巧 (Tips and Tricks)

### 1. 事件频率与性能开销

生产最大顾虑：**BPF 拖慢系统**。

**三因素：**

```
开销 ∝ (事件频率 × 每事件动作成本) / CPU 数量
```

| 因素 | 说明 |
|------|------|
| **频率** | 每秒触发次数 — 差 **数量级** |
| **动作** | 计数 vs `printf` vs uprobe 读栈 |
| **CPU 数** | 多核分摊 per-CPU buffer，但全局竞争仍在 |

**频率对比（直觉）：**

| 事件 | 约频率 | 开销直觉 |
|------|--------|----------|
| 线程休眠、**exec** | 几次/秒 | 可忽略 |
| **syscall** 聚合 | 千–百万/秒 | 看工具 |
| **每包 kprobe**、每函数 **trace** | 百万–千万/秒 | **极端** — 勿生产常开 |

**探针类型成本（书中测试趋势）：**

| 类型 | 相对成本 |
|------|----------|
| **kprobe** | 较低 |
| tracepoint | 低（稳定） |
| **uprobe / uretprobe** | **最高** — 单事件可 **>1µs** 级 |

**HFT 纪律：**

- 热路径：**聚合 Map**（`count`/`hist`）— [Ch 4](./chapter-04-BCC.md)  
- 禁：**trace` 逐行**、高频 **uprobe**  
- **短窗口、限 PID、限核**

---

### 2. 以 49 或 99 Hz 采样

CPU 剖析（`profile`、perf）— **勿用整数 100 Hz**。

| 问题 | 说明 |
|------|------|
| **锁步采样 (lockstep)** | 应用每 **10ms** 定时任务 + **100Hz** 采样 → 永远采到或永远采不到 → **扭曲** |
| **对策** | **99Hz 或 49Hz** — 与周期 **互质** |

```bash
sudo profile-bpfcc -F 99 30
perf record -F 99 -a -g -- sleep 30
```

→ [Ch 6 § profile](./chapter-06-CPU.md) · [Ch 2 § 火焰图](./chapter-02-技术背景.md)

---

### 3. 黄猪与灰鼠：特殊数字法

**场景：** 知道行为（如「某次 write」）但 **不知内核函数名**。

**步骤：**

1. 写测试程序，**精确执行 N 次**（如 **230,000**）目标操作  
2. N 用 **罕见计数** — 书中用 **23（灰鼠）/ 17（黄猪）** 等质数组合，系统中 **自然出现概率极低**  
3. 同时跑 **`funccount`** 统计内核函数  
4. 在输出中 **搜 `230000`** — 瞬间定位 **目标函数**

```bash
# 终端 A：全函数计数（短跑）
sudo funccount-bpfcc -r '^[a-z_]+$' &
# 终端 B：跑 230000 次 write 的小程序
./my_230000_writes
# 在 funccount 结果里 grep 230000
```

**本质：** **可控实验** + **全局搜索** — 比盲读内核源码快。

---

### 4. 编写目标软件 (Write Target Software)

| 误区 | 对策 |
|------|------|
| 只在庞大生产系统上试 | **先写最小负载生成器** |
| 不懂 struct/参数 | 自己写代码 → 知 **参数含义、返回值** |

**HFT：** 复现 **单 syscall、单 connect** 的 microbench，再挂 BPF — 与 [15-HFT ch10 压测](../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md) 同思路。

---

### 5. 学习系统调用 (Learn Syscalls)

**syscall = 用户态与内核边界** — tracepoint 字段来自 **内核 syscall 入口定义**。

```bash
man 2 read
man 3 getaddrinfo
man 2 setitimer
ls /sys/kernel/debug/tracing/events/syscalls/
```

**收益：** 写 bpftrace 时知道 **`args->` 有什么** — 少猜 struct。

→ [07-TLPI](../07-The-Linux-Programming-Interface/) · [Ch 5 bpftrace](./chapter-05-bpftrace.md)

---

### 6. 保持简单 (Keep It Simple)

| 诱惑 | 后果 |
|------|------|
| 一个脚本挂 **所有 probe** | 高开销、难维护、难解释 |
| **最好工具** | 只追 **回答当前问题** 的最少事件 |

**Gregg 原则：** 与 Unix 单用途工具哲学一致 — [Ch 4 § 单用途](./chapter-04-BCC.md)。

---

## 第二部分：常见问题与修复 (Common Problems)

### 1. 丢失的事件 (Missing Events)

**挂了 probe 无输出：**

| 原因 | 验证 |
|------|------|
| 事件 **未发生** | `perf stat -e tracepoint:...` 或 `funccount` |
| **静态链接** | 无 PLT — uprobe 挂 libc 失败 |
| **直接 syscall** | 绕过 libc wrapper |
| 过滤器太严 | bpftrace `/filter/` |

```bash
perf stat -e 'syscalls:sys_enter_openat' sleep 5
```

---

### 2. 缺失 / 断裂的堆栈 (Missing Stack Traces)

**现象：** 栈只有 1–2 帧 + **`[unknown]`**。

**根因：** BPF 默认 **帧指针 (frame pointer)** walk — 编译器常 **默认 `-fomit-frame-pointer`**（省略帧指针以微优化），**RBP 不再链栈帧**。

| 错说法 | 正说法 |
|--------|--------|
| 「开了 `-fno-omit-frame-pointer` 导致断栈」 | **omit（省略）帧指针** 导致断栈；修复是 **`-fno-omit-frame-pointer`（不要省略）** |

**修复：**

| 方案 | 说明 |
|------|------|
| **`-fno-omit-frame-pointer`** | 应用/策略二进制 — [Ch 12](./chapter-12-语言.md) |
| **debuginfo + DWARF** | 更准更慢 |
| **ORC / LBR**（内核/perf 能力） | 环境相关、演进中 |
| **libc 断在 `read+0x…`** | 发行版 **libc 无 FP** — [Ch 13 §10](./chapter-13-应用程序.md) |

**HFT 发布链：** 策略 **.so 必须保留 FP** — 否则 Off-CPU/火焰图 **半盲**。

---

### 3. 缺失符号名 (Missing Symbols)

**现象：** 仅地址 / `[unknown]`，无函数名。

| 环境 | 对策 |
|------|------|
| **JIT (Java/Node)** | **`/tmp/perf-PID.map`** + `jmaps` / perf-map-agent — **实时** 生成 — [Ch 12](./chapter-12-语言.md) |
| **C/C++ ELF** | 勿 **strip**；装 **debuginfo** 包 |
| 内核栈 | `linux-image-*-dbgsym` |

```bash
readelf -s ./my_strategy | head
file ./my_strategy   # not stripped?
```

---

### 4. 追踪时找不到函数 (Missing Functions)

**uprobe 报找不到符号：**

| 原因 | 对策 |
|------|------|
| **内联 (inlining)** | 函数 **不存在独立符号** — 追 **父函数/子函数** |
| 名称 mangling (C++) | `c++filt` |
| 静态函数 | 无导出符号 — 用偏移或换 tracepoint |

**编译验证：** `objdump -d` 看是否只剩父函数体。

---

### 5. 反馈循环 (Feedback Loops) ⚠️

**永远不要追踪「追踪器自身输出路径」。**

| 例子 | 结果 |
|------|------|
| BPF **`printf`** + 同时 trace **`sys_write`** | write → printf → write → **风暴/崩溃** |
| 日志系统 + `write` uprobe | 同理 |

**对策：** 聚合 Map 代替 printf；或 **过滤 trace 自身 PID**；生产用 **结束时的 map 打印**。

---

### 6. 丢弃的事件 (Dropped Events)

**输出/缓冲跟不上事件率：**

| 症状 | 说明 |
|------|------|
| `WARNING: N stack traces could not be displayed` | perf ring buffer 满 |
| map 丢样 | 工具内部限制 |

**修复：**

- **降频** — 换 `funccount`/`hist` 代替 `trace`  
- **增 buffer** — 工具参数 / `sysctl kernel.perf_event_max_stack` 等（视工具）  
- **缩短采集窗口**  
- **只追单 PID**

---

## 3. 问题速查表

| 现象 | 优先查 |
|------|--------|
| 系统变卡 | 频率 × 动作 — §1 |
| 火焰图偏一种任务 | 是否 **100Hz 锁步** — §2 |
| 不知内核函数名 | **黄猪/灰鼠 + funccount** — §3 |
| 无 probe 输出 | **perf stat 验证** — §6.1 |
| 栈 `[unknown]` | **帧指针 / libc** — §6.2 |
| 无函数名 | **strip / JIT map** — §6.3 |
| uprobe 无符号 | **内联** — §6.4 |
| 工具拖垮系统 | **反馈循环** — §6.5 |
| WARNING dropped | **降频/缩窗** — §6.6 |

---

## 4. 全书回顾与 HFT 最小 runbook

**Part I：** 懂 BPF（Ch 1–5）  
**Part II：** 按资源查（Ch 6–16）  
**Part III：** 生态 + **心法（Ch 17–18）**

**HFT incident 最小集（遵守 Ch 18 纪律）：**

```
1. 业务 histogram 锁定段
2. Linux 60s（Ch 3）
3. runqlat 10s · tcpretrans 30s · profile -F 99 30s（Ch 6/10）
4. 若 CPU 不高 → offcputime（Ch 13）
5. bpftrace 验证单点（Ch 5）— 短跑
6. 全程：限 PID · 限时 · 热路径核慎挂
```

**构建链 checklist（Ch 18 + 12 + 13）：**

- [ ] `-fno-omit-frame-pointer`  
- [ ] debuginfo 可装  
- [ ] 无高频 uretprobe（Go）  
- [ ] 热路径无 `trace`/printf 式 BPF  

---

## 5. HFT 读者 Takeaway

1. **开销公式** 刻进 runbook — **频率** 是第一旋钮。  
2. **99Hz** — 所有 CPU 剖析默认。  
3. **黄猪/灰鼠** — 内核函数探索的 **实验科学** 法。  
4. **帧指针 + 符号** — 比多学两个工具更重要。  
5. **简单** — 一个假设一个脚本。  
6. **反馈循环** — 生产事故常见自毁类型。  
7. 全书工具索引 → [OUTLINE.md](./OUTLINE.md) · 单行 → [附录 A](./appendix-A-bpftrace单行命令.md)。

---

## 相关章节

- 上一章：[chapter-17-其他BPF工具.md](./chapter-17-其他BPF工具.md)
- 附录 A：[appendix-A-bpftrace单行命令.md](./appendix-A-bpftrace单行命令.md)
- 技术地基：[chapter-02-技术背景.md](./chapter-02-技术背景.md)
- 语言/符号：[chapter-12-语言.md](./chapter-12-语言.md)
- libc 断栈：[chapter-13-应用程序.md](./chapter-13-应用程序.md)
- BCC 调试：[chapter-04-BCC.md](./chapter-04-BCC.md)
- 方法论：[chapter-03-性能分析.md](./chapter-03-性能分析.md)
- SysPerf 基准：[chapter-12-benchmarking](../02-Systems-Performance-2nd/chapter-12-benchmarking/)
