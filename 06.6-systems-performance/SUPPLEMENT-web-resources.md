# 网络搜索补充：2020 年后系统性能生态演进与 HFT 关联

> **来源：** Brendan Gregg 官方博客与书籍页面、kernel.org、LWN.net、LKML、bpftrace/BCC/libbpf changelog、HFT 行业报道
> **搜索日期：** 2026-08-16
> **目的：** 补充原书（2020 年出版，内核 5.x 时代）未覆盖的生态变化，并标注 HFT 关联

---

## 一、官方书籍资源

### 1.1 书籍信息

| 项目 | 内容 |
|------|------|
| 书名 | Systems Performance: Enterprise and the Cloud, 2nd Edition |
| 作者 | Brendan Gregg |
| 出版 | Addison-Wesley, 2020 |
| 官方页面 | [brendangregg.com/systems-performance-2nd-edition-book.html](https://www.brendangregg.com/systems-performance-2nd-edition-book.html) |
| 博客发布文 | [blog/2020-07-15](https://www.brendangregg.com/blog/2020-07-15/systems-performance-2nd-edition.html) |
| 第一版 | 2013 年出版，已被翻译为中、日、波兰、韩文 |
| 姊妹书 | [BPF Performance Tools](https://www.brendangregg.com/bpf-performance-tools-book.html)（2019） |

### 1.2 勘误（Errata）

官方页面列出第 1 次印刷的勘误（最后更新 2020-01-11）。感谢技术编辑 Deirdré Straughan。目前官方未公布大规模勘误列表，说明内容质量较高。

### 1.3 第二版 vs 第一版主要变化

- **新增：** BPF、BCC、bpftrace、perf、Ftrace 章节（第 13–15 章）
- **移除：** Solaris 相关内容（大部分）
- **更新：** Linux 内核与云计算内容
- **改进：** 基于 Netflix 6 年性能工程经验的改进

Gregg 在博客中发布了[全书可视化](http://www.brendangregg.com/blog/images/2020/SystemsPerformance2ndEdition-draft15jul20-entirebook.jpg)，用颜色标注了改动范围——前几章改动少（持久技能），后几章改动大（工具与调优）。

### 1.4 Brendan Gregg 职业变动

| 时间 | 职位 | 说明 |
|------|------|------|
| 2014–2022 | Netflix 高级性能工程师 | 云性能分析，本书主要经验来源 |
| 2022–2025 | Intel 性能工程师 | 处理器性能、AI Flame Graphs |
| 2025–至今 | OpenAI 数据中心性能 | ChatGPT 基础设施性能优化 |

> **HFT 关联：** Gregg 在 Netflix 的工作聚焦云环境延迟优化，其方法论（USE 方法、USE 方法、火焰图）完全适用于 HFT 系统分析。

---

## 二、2020 年后 Linux 内核性能特性

原书写作时主流内核为 5.4–5.8。以下按内核版本梳理关键性能特性：

### 2.1 内核版本时间线

| 内核 | 发布 | 性能关键特性 | HFT 关联 |
|------|------|-------------|----------|
| 5.9 | 2020-10 | `bpf_link` 持久化附着 | 工具不再需要常驻进程 |
| 5.10 | 2020-12 | **CPU 频率不变性改进**；io_uring 高性能异步 IO | io_uring：HFT 日志写入异步化 |
| 5.11 | 2021-02 | AMD CPPC 驱动（更精细频率控制） | AMD 处理器频率管理 |
| 5.12 | 2021-03 | BPF 睡眠支持；`bpf_copy_from_user()` | 内核探针中安全做用户态拷贝 |
| 5.13 | 2021-06 | AMD P-State 驱动（acpi-cpufreq 替代） | AMD Zen 3/4 频率响应速度提升 |
| 5.14 | 2021-08 | `bpf_timer`；`bpf_loop()`；task candidate 调度器 | — |
| 5.15 | 2021-10 | `bpf_get_branch_snapshot`（LBR 快照） | **HFT：捕获分支预测失败路径** |
| 5.16 | 2022-01 | io_uring CQE-skip；KF_FAST 汇编优化 | — |
| 5.18 | 2022-05 | AMD P-State EPP 默认；NUMA 平衡改进 | NUMA 感知对 HFT 内存延迟关键 |
| 6.0 | 2022-10 | `bpf_loop()` 正式；BPF_MAP_TYPE_USER_RINGBUF | — |
| 6.1 | 2022-12 | Rust 支持；io_uring 零拷贝缓冲注册 | — |
| 6.2 | 2023-02 | `bpf_arena` 初步讨论；AMD P-State 默认 | AMD 处理器更智能频率管理 |
| 6.3 | 2023-04 | cacheless page table 编译选项；NUMA 改进 | — |
| 6.6 (LTS) | 2023-10 | io_uring NAPI busypoll；BPF arena 初步 | **io_uring NAPI：低延迟 IO 路径** |
| 6.7 | 2024-01 | 异常处理 `bpf_throw()`；Netkit（替代 veth） | 容器网络性能接近宿主机 |
| 6.8 | 2024-03 | 文件验证（LSM + fsverity）；验证器改善 | — |
| 6.9 | 2024-05 | **BPF Arena**；**BPF Token**（非特权容器） | Arena：高吞吐内核↔用户态数据交换 |
| 6.10 | 2024-07 | 工作队列异步执行；**KPROBE 会话** | **函数耗时测量一步到位** |
| 6.11 | 2024-09 | 新 uretprobe 系统调用（**x86_64 快 10–30%**） | **uretprobe 提速直接利好 HFT 用户态插桩** |
| 6.12 (LTS) | 2024-11 | **PREEMPT_RT 合入主线**；**sched_ext**；EEVDF | **HFT 里程碑：主线实时内核 + BPF 自定义调度** |

### 2.2 重点特性详解

#### PREEMPT_RT（Linux 6.12）

经过 20 年开发，PREEMPT_RT 终于合入主线。内核代码路径全部可抢占，提供确定性低延迟。对 HFT 意义重大——不再需要打实时补丁树。

- **架构支持：** x86_64、ARM64、RISC-V
- **HFT 影响：** 消除不可抢占段导致的微秒级抖动
- **原书位置：** 第 6 章 6.5.6 节"实时优先级"提到 PREEMPT_RT，现为内核主线特性

#### sched_ext（Linux 6.12）

BPF 可编程调度器类。开发者可以用 BPF 程序定义任务调度算法，无需修改内核源码。

- **文档：** `Documentation/scheduler/sched-ext.rst`
- **HFT 场景：** 编写 BPF 调度器把交易线程绑定到独占核，避免调度器抖动
- **原书位置：** 第 6 章 6.5 节"调度器"，原书未提及 BPF 可编程调度

#### AMD P-State（Linux 5.13+）

AMD P-State 驱动替代旧 acpi-cpufreq，提供更精细的频率控制：

- **CPPC（Collaborative Processor Performance Control）：** 直接向 CPU 发频率请求，绕过 ACPI 表
- **EPP（Energy Performance Preference）：** 用户态可设置偏好，更快的频率响应
- **HFT 影响：** AMD Zen 3/4 处理器频率切换延迟从毫秒级降至微秒级

#### io_uring（Linux 5.10+）

高性能异步 IO 接口，原书仅简要提及。截至 2024 已大幅演进：

- **零拷贝缓冲注册：** `IORING_REGISTER_PBUF_RING`
- **NAPI busypoll：** 减少 syscall 开销
- **HFT 应用：** 异步日志写入、配置文件读取不阻塞交易路径

#### EEVDF 调度器（Linux 6.6+）

Earliest Eligible Virtual Deadline First 替代 CFS：

- 更精确的延迟控制
- 优先级更公平的 CPU 时间分配
- **HFT 影响：** 交易线程延迟可预测性提升

---

## 三、perf 工具演进

原书第 13 章基于 perf 5.x。截至 2025 年 perf 已大幅增强：

### 3.1 数据类型分析（Data-Type Profiling）

Linux 6.10–6.12 引入了基于 DWARF 和 BTF 的数据类型分析：

```bash
# 按数据类型分类显示热点
perf report -s type,typecln,typeoff -H

# 示例输出：
# 2.67% struct cfs_rq
# 1.23% struct cfs_rq: cache-line 2
# 0.57% struct cfs_rq: cache-line 4
# 0.39% struct cfs_rq +0x14 (h_nr_running)
```

- **缓存行分析：** `typecln` 排序模式显示热/冷缓存行
- **寄存器追踪：** 跟踪 add/sub/lea 运算的寄存器偏移
- **全局变量：** 支持全局变量类型解析
- **HFT 价值：** 精确定位缓存行争用，优化数据结构布局

### 3.2 perf trace 增强

- 使用 BPF + BTF 收集和美化系统调用/tracepoint 参数（GSoC 项目）
- 支持 `--addr2line` 选项
- 支持 capstone 反汇编库

### 3.3 其他 perf 改进

| 特性 | 内核版本 | 说明 |
|------|---------|------|
| Capstone 反汇编器 | 6.10 | 替代 objdump，更快更准 |
| LLVM 反汇编 | 6.12 | 使用 LLVM 库加速反汇编和 addr2line |
| PowerPC 支持 | 6.12 | 初始 PowerPC 支持 |
| AMD Zen 5 事件 | 6.10 | Zen 5 核心和 uncore 事件 |
| Intel 事件更新 | 6.10 | Emerald Rapids、Sierra Forest 等 |

---

## 四、Ftrace 更新

原书第 14 章基于 Ftrace 5.x。主要变化：

### 4.1 trace-cmd 和 KernelShark

- **trace-cmd v3.x：** 改进了 XML/JSON 导出、Python 绑定
- **KernelShark：** GUI 工具持续改进，支持更复杂的过滤和可视化

### 4.2 Ftrace 内核特性

| 特性 | 内核 | 说明 |
|------|------|------|
| `function-fork` 选项 | 5.10 | 只追踪 fork 出的子进程 |
| `trace_printk` 改进 | 5.14 | 性能改善 |
| `hist` 触发器增强 | 5.15+ | 支持复合键、多值合成 |
| `event/*` 通配符 | 5.18 | 更灵活的事件匹配 |
| BPF 替代 | 6.0+ | 许多 Ftrace 用途已被 BPF 工具取代 |

> **原书评价：** Gregg 在第 14 章提到 Ftrace 是"隐藏的开关"（hidden light switch）。2020 年后 BPF 工具（bpftrace、BCC）已成为更高级的替代，但 Ftrace 仍是最轻量的内核追踪手段。

---

## 五、BPF 工具演进概览

> 详细 BPF 生态演进请参阅姊妹书笔记目录下的 [`SUPPLEMENT-web-resources.md`](../06.7-bpf-observability/bpf-performance-tools/SUPPLEMENT-web-resources.md)。

### 5.1 关键变化摘要

| 维度 | 原书状态（2020） | 现状（2025） |
|------|-----------------|-------------|
| bpftrace | v0.9 | v0.25（宏、Record 类型、import 语句） |
| BCC | Python 接口为主 | libbpf-tools（C 版）为主，Python 接口已 deprecated |
| libbpf | 0.x | 1.6（BPF token、arena、blazesym） |
| CO-RE | 早期 | 主流发行版默认启用 BTF |
| 内核 BPF | 5.x | 6.12（PREEMPT_RT、sched_ext、KPROBE 会话） |

### 5.2 Linux Crisis Tools（2024）

Gregg 在 2024 年 3 月发布 [Linux Crisis Tools](https://www.brendangregg.com/blog/2024-03-24/linux-crisis-tools.html) 博文，推荐在 Linux 服务器上**预装**以下工具（原书表 4.1 的扩展版）：

```bash
# Ubuntu/Debian 一键安装
sudo apt install procps util-linux sysstat iproute2 numactl \
  tcpdump linux-tools-common linux-tools-$(uname -r) \
  bpfcc-tools bcc bpftrace trace-cmd nicstat ethtool \
  tiptop cpuid msr-tools
```

| 包名 | 提供工具 | 用途 |
|------|---------|------|
| procps | ps, vmstat, uptime, top | 基本统计 |
| util-linux | dmesg, lsblk, lscpu | 系统日志、设备信息 |
| sysstat | iostat, mpstat, pidstat, sar | 设备统计 |
| iproute2 | ip, ss, nstat, tc | 网络配置 |
| numactl | numastat | NUMA 统计 |
| tcpdump | tcpdump | 网络抓包 |
| linux-tools-common | perf, turbostat | 性能剖析 |
| bpfcc-tools | opensnoop, execsnoop, runqlat, biolatency 等 | eBPF 工具集 |
| bpftrace | bpftrace | eBPF 脚本语言 |
| trace-cmd | trace-cmd | Ftrace CLI |
| nicstat | nicstat | 网络设备统计 |
| ethtool | ethtool | 网络设备信息 |
| tiptop | tiptop | PMU/PMC top |
| cpuid | cpuid | CPU 详情 |
| msr-tools | rdmsr, wrmsr | CPU 寄存器读写 |

> **HFT 部署建议：** 以上工具仅占几 MB 空间，应在交易服务器镜像中预装。出故障时再安装可能因系统负载过高而无法安装。

### 5.3 帧指针回归（2024）

Gregg 发表 [The Return of the Frame Pointers](https://www.brendangregg.com/blog/2024-03-17/the-return-of-the-frame-pointers.html) 博文：

- **问题：** 过去 20 年 GCC 默认 `-fomit-frame-pointer`，导致性能分析工具（perf、火焰图）无法准确回溯栈
- **解决：** Fedora 38+ 和 Ubuntu 24.04+ 已重新启用帧指针
- **影响：** 性能剖析质量大幅提升，火焰图更准确
- **HFT 价值：** 交易引擎 C++ 代码编译时应确保 `-fno-omit-frame-pointer`，以便低开销剖析

### 5.4 Fast by Friday 方法论（2023）

Gregg 在 eBPF Summit 2023 提出"Fast by Friday"方法论：

- 目标：一周内找到并解决性能问题
- 核心步骤：
  1. 预装危机工具
  2. 用 USE 方法快速扫描
  3. 用火焰图定位热点
  4. 用 bpftrace 验证假设
  5. 逐项消除瓶颈

---

## 六、Brendan Gregg 2020 年后重要博文与演讲

### 6.1 博文时间线

| 日期 | 标题 | 主题 | HFT 关联 |
|------|------|------|----------|
| 2020-07-15 | Systems Performance 2nd Edition | 书籍发布 | — |
| 2020-11-04 | BPF binaries: BTF, CO-RE, and the future | BPF 工具可移植性 | — |
| 2021-05-23 | What is Observability | 可观测性定义 | — |
| 2021-06-03 | How To Add eBPF Observability To Your Product | 产品集成 eBPF | — |
| 2021-07-05 | Computing Performance: On the Horizon | LISA2021 主题演讲 | 硬件性能趋势 |
| 2021-08-27 | Slack's Secret STDERR Messages | eBPF 调试案例 | — |
| 2021-08-30 | Analyzing a High Rate of Paging | 分页分析 | **HFT 内存分析参考** |
| 2021-09-06 | ZFS Is Mysteriously Eating My CPU | ZFS 性能问题 | — |
| 2022-04-19 | TensorFlow Library Performance | TensorFlow 性能 | — |
| 2022-05-02 | Brendan@Intel.com | 加入 Intel | — |
| 2023-03-01 | Computing Performance: What's on the Horizon | SREcon APAC 2022 | **硬件性能预测** |
| 2023-04-28 | eBPF Observability Tools Are Not Security Tools | eBPF 安全边界 | — |
| 2024-03-10 | eBPF Documentary | eBPF 纪录片 | — |
| 2024-03-17 | The Return of the Frame Pointers | 帧指针回归 | **剖析质量提升** |
| 2024-03-24 | Linux Crisis Tools | 危机工具列表 | **HFT 服务器预装清单** |
| 2024-07-22 | No More Blue Fridays | eBPF 安全内核更新 | — |
| 2024-10-29 | AI Flame Graphs | AI/GPU 火焰图 | — |
| 2025-02-07 | Why I joined OpenAI | 加入 OpenAI | — |

### 6.2 关键演讲

| 会议 | 演讲 | 要点 |
|------|------|------|
| LISA 2021 | BPF Internals (eBPF) | 122 页幻灯片，从 bpftrace 到机器码全流程 |
| eBPF Summit 2021 | Performance Wins with eBPF | eBPF 入门指南 |
| SREcon APAC 2022 | Computing Performance: What's on the Horizon | 处理器/内存/磁盘/网络/虚拟化未来 |
| eBPF Summit 2023 | Fast by Friday | 一周解决性能问题方法论 |
| Kernel Recipes 2023 | Fast by Friday (updated) | 更新版 |

---

## 七、HFT 性能分析生态

### 7.1 HFT 内核优化方案

原书面向云/企业环境，以下补充 HFT 专用的内核性能优化：

#### CPU 层面

| 优化 | 方法 | 效果 |
|------|------|------|
| CPU 隔离 | `isolcpus=` + `nohz_full=` | 独占核，无调度器干扰 |
| 实时调度 | PREEMPT_RT（6.12 主线） | 确定性低延迟 |
| 频率锁定 | `cpufreq=performance` + 禁用 C-state | 消除频率切换抖动 |
| 超线程关闭 | BIOS 设置 | 消除共享资源争用 |
| NUMA 绑定 | `numactl --cpunodebind=0 --membind=0` | 本地内存访问 |

#### 网络层面

| 技术 | 延迟 | 说明 |
|------|------|------|
| 标准 Linux 内核栈 | 20–50µs | 原书第 10 章内容 |
| OpenOnload (Solarflare) | 3–10µs | 透明内核旁路，BSD socket 兼容 |
| DPDK | 1–5µs | 用户态驱动，忙轮询，需重写应用 |
| RDMA (Mellanox) | 1–2µs | 硬件级内存到内存传输 |
| ExaNIC (Exablaze) | 200–800ns | FPGA 加速，极致低延迟 |
| XDP/eBPF | µs 级 | 内核内快速路径，适合流量过滤 |

> **原书关联：** 第 10 章网络性能分析的方法论（USE 方法、延迟分析）完全适用于内核旁路环境。第 11 章云计算中 Nitro Enclaves 等技术也可用于 HFT 云部署。

#### 内存层面

| 优化 | 方法 | 效果 |
|------|------|------|
| 透明大页 | THP 启用（或禁用，视场景） | 减少 TLB miss |
| 内存锁定 | `mlockall()` | 防止 swap 导致延迟尖峰 |
| NUMA 亲和 | 内存分配在同 NUMA 节点 | 减少跨节点访问延迟（60→100ns） |
| 内存池 | 预分配 + 对象池 | 避免运行时分配抖动 |

### 7.2 HFT 专用 Linux 内核

**QuantKernel**（[quantkernel.org](https://quantkernel.org/zh)）是基于 Linux 6.12 LTS 的 HFT 专用内核：

- 平均延迟 <10µs，P99.9 延迟 <50µs
- PREEMPT_RT 实时补丁，1000Hz 高频时钟
- NO_HZ_FULL 全动态无滴答，CPU 隔离绑核
- 禁用 Spectre/Meltdown 缓解措施（性能优先）
- 支持 eBPF XDP、Mellanox VMA、Solarflare Onload、DPDK

### 7.3 HFT 性能分析工具组合

| 用途 | 工具 | 说明 |
|------|------|------|
| 系统级监控 | sar + sysstat | 原书附录 B，历史性能数据 |
| CPU 剖析 | perf + 火焰图 | 原书第 13 章 |
| 内核追踪 | bpftrace / BCC | 原书第 15 章 |
| Ftrace 追踪 | trace-cmd | 原书第 14 章 |
| 网络延迟 | tcpdump + tcptop (BCC) | 原书第 10 章 |
| 系统调用 | strace / perf trace | 原书第 4 章 |
| USE 方法 | USE 工具表 | 原书附录 A |
| HFT 专用 | Solarflare OpenOnload 分析工具 | 内核旁路性能分析 |
| FPGA | Xilinx Vitis / Intel oneAPI | 硬件加速性能分析 |

### 7.4 HFT 延迟预算

竞争性 HFT 系统的典型延迟预算：

```
市场数据接收 (500ns) → 行情解析 (200ns) → 策略决策 (800ns) → 风控 (100ns) → 订单发送 (1µs)
总计: ~2.6µs tick-to-trade
```

各组件必须在分配内完成。如果网络栈耗时 10µs，决策质量无关紧要——栈吃掉了优势。

### 7.5 HFT 内核旁路的瓶颈迁移

HFT 行业经验表明，解决一个瓶颈后瓶颈会迁移：

1. **网络栈（50K msg/s 以下）：** 内核网络是约束 → 内核旁路解决
2. **订单处理器（120K msg/s）：** 单线程订单处理器饱和 → 多线程扩展
3. **订单簿管理（200K+ msg/s）：** 订单簿更新成为约束 → 优化数据结构
4. **风控引擎：** 风控检查成为约束 → 预计算 + 批量化

> **原书关联：** 第 2 章 2.5.7 节"瓶颈迁移"理论在 HFT 中表现得尤为明显。

---

## 八、perf 数据类型分析深入（2024–2025）

### 8.1 数据类型分析演进

原书第 13 章未覆盖的 perf 数据类型分析功能：

| 内核版本 | 改进 | 说明 |
|---------|------|------|
| 6.10 | 全局变量类型解析 | 保持缓存加速查找 |
| 6.10 | `call` 指令处理 | 处理寄存器效果和返回值 |
| 6.10 | x86 段地址支持 | `%gs:0x28` 等per-CPU 变量 |
| 6.10 | Capstone 反汇编 | 替代 objdump，大幅加速 |
| 6.10 | AMD Zen 5 事件 | 新处理器支持 |
| 6.12 | `typecln` 排序 | 显示热/冷缓存行 |
| 6.12 | LLVM 反汇编 | 使用 LLVM 库加速 |
| 6.12 | `typecln` + `typeoff` | 缓存行内偏移显示 |
| 2025-08 | 寄存器偏移追踪 | add/sub/lea 运算跟踪 |
| 2025-10 | DW_OP_piece 支持 | 分段变量表达式 |
| 2025-10 | Rust 命名空间修复 | Rust 程序类型分析 |

### 8.2 实用命令

```bash
# 按数据类型分类显示热点
perf report -s type,typecln,typeoff -H

# 数据类型标注（TUI）
perf annotate --data-type

# 事件组显示（内存 load + store）
perf record -e cpu/mem-loads/,cpu/mem-stores/ ...
perf report -s type --group
```

---

## 九、与原书的对照阅读建议

| 原书章节 | 网络补充重点 |
|---------|-------------|
| 第 1 章 概述 | Gregg 2025 年加入 OpenAI，性能分析范围扩展到 AI |
| 第 2 章 方法论 | 补读：Fast by Friday 方法论（本文 5.4） |
| 第 3 章 操作系统 | 补读：PREEMPT_RT 合入主线、EEVDF 调度器（本文 2.2） |
| 第 4 章 观测工具 | 补读：Linux Crisis Tools 完整列表（本文 5.2） |
| 第 5 章 应用 | 补读：帧指针回归对应用剖析的影响（本文 5.3） |
| 第 6 章 CPU | 补读：sched_ext、AMD P-State、PREEMPT_RT（本文 2.2） |
| 第 7 章 内存 | 补读：NUMA 平衡改进、THP 争议（本文 7.1） |
| 第 8 章 文件系统 | 补读：XFS self-healing（Linux 7.0 内核） |
| 第 9 章 磁盘 | 补读：NVMe over TCP、io_uring 块 IO |
| 第 10 章 网络 | 补读：内核旁路技术对比（本文 7.1） |
| 第 11 章 云计算 | 补读：Nitro Enclaves、容器网络改进 |
| 第 12 章 基准测试 | 补读：HFT 微基准陷阱（本文 7.5） |
| 第 13 章 perf | 补读：数据类型分析（本文 3.1、8.1） |
| 第 14 章 Ftrace | 补读：BPF 替代趋势（本文第四节） |
| 第 15 章 BPF | **详细参考姊妹书 SUPPLEMENT**（本文第五节） |
| 第 16 章 案例 | 补读：Gregg 博客实战案例（本文 6.1） |
| 附录 A USE 方法 | 仍为最佳实践，无重大变化 |
| 附录 B sar | sar 仍是最基础的历史性能数据工具 |
| 附录 C bpftrace | **参考姊妹书 SUPPLEMENT 第四节** bpftrace 0.9→0.25 变化 |
| 附录 D 习题 | 无变化 |
| 附录 E 人物 | 补充：Gregg 2025 年加入 OpenAI |

---

## 十、参考链接

### 官方资源

- [Systems Performance 2nd Edition 官方页面](https://www.brendangregg.com/systems-performance-2nd-edition-book.html) — 书籍信息、勘误
- [Brendan Gregg 博客](https://www.brendangregg.com/blog/) — 2020 年后所有博文
- [Brendan Gregg 主页](https://www.brendangregg.com/) — 所有文档、视频、软件索引
- [Linux Performance](https://www.brendangregg.com/linuxperf.html) — Linux 性能资源汇总

### 内核与工具

- [KernelNewbies](https://kernelnewbies.org/) — 每个内核版本特性详解
- [perf tools Git](https://git.kernel.org/pub/scm/linux/kernel/git/perf/perf-tools.git/) — perf 工具源码
- [bpftrace CHANGELOG](https://github.com/iovisor/bpftrace/blob/master/CHANGELOG.md) — bpftrace 版本历史
- [BCC libbpf-tools](https://github.com/iovisor/bcc/tree/master/libbpf-tools) — C 版 BCC 工具

### HFT 性能

- [QuantKernel](https://quantkernel.org/zh) — HFT 专用 Linux 内核
- [Kernel Bypass Networking for HFT](https://www.quantlabsnet.com/post/kernel-bypass-networking-for-ultra-low-latency-hft-systems) — 内核旁路技术对比
- [The Nanosecond Economy](https://www.nikhilpadala.com/blog/nanosecond-economy-hft-infrastructure) — HFT 基础设施基础
- [CPU Optimization for HFT](https://www.linkedin.com/pulse/cpu-optimization-linux-ultra-low-latency-trading-nikhil-goud-5rezc) — CPU 层面优化
- [The Kernel-Bypass Bottleneck Trap](https://electronictradinghub.com/the-kernel-bypass-bottleneck-trap-why-2m-in-hft-infrastructure-does-not-fix-your-latency-problem) — 瓶颈迁移分析

### 姊妹书补充

- [BPF Performance Tools SUPPLEMENT](../06.7-bpf-observability/bpf-performance-tools/SUPPLEMENT-web-resources.md) — BPF 生态 2019 年后演进详解
