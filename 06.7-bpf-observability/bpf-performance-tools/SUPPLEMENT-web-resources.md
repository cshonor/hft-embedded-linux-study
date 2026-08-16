# 网络搜索补充：2019 年后 BPF 生态演进与 HFT 关联

> **来源：** Brendan Gregg 官方书籍页面、GitHub 仓库、bpftrace/BCC/libbpf changelog、kernelnewbies.org、LWN.net、HFT 行业报道
> **搜索日期：** 2026-08-16
> **目的：** 补充原书（2019 年出版，内核 4.x 时代）未覆盖的生态变化，并标注 HFT 关联

---

## 一、官方书籍资源

### 1.1 工具源码仓库

| 仓库 | 说明 |
|------|------|
| [brendangregg/bpf-perf-tools-book](https://github.com/brendangregg/bpf-perf-tools-book) | 官方工具仓库，`originals/`（原始版）、`updated/`（修复版）、`exercises/`（练习解答） |
| [iovisor/bcc](https://github.com/iovisor/bcc) | BCC 主仓库，含 libbpf-tools（C 版，推荐） |
| [iovisor/bpftrace](https://github.com/iovisor/bpftrace) | bpftrace 主仓库，含工具集和参考指南 |
| [libbpf/libbpf](https://github.com/libbpf/libbpf) | libbpf 主仓库（从 Linux 内核源码树独立） |

### 1.2 关键文档

- bpftrace 参考指南：[reference_guide.md](https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md)
- bpftrace 迁移指南（Breaking Changes）：[migration_guide.md](https://github.com/bpftrace/bpftrace/blob/master/docs/migration_guide.md)
- BCC 安装说明：[INSTALL.md](https://github.com/iovisor/bcc/blob/master/INSTALL.md)
- Brendan Gregg eBPF 页面：[brendangregg.com/ebpf.html](https://www.brendangregg.com/ebpf.html)

### 1.3 书籍勘误（Errata）

以下勘误影响技术理解，按重要性排序：

| 位置 | 错误 | 修正 |
|------|------|------|
| 2.13, p64 | "Linux 2.6.21" | Linux 2.6.31 |
| 9.3.7, p370 | `kprobe:blk_start_request,kprobe:blk_mq_start_request` | `kprobe:blk_account_io_done` |
| 6.2.3, p192 | "perf list"（出现两次） | "perf script" |
| 6.2.5, p196 | "perf script to show the rate" | "perf stat" |
| 9.3.2, p359 | "biostoop(8)" | "biosnoop(8)" |
| 附录 E, p786 | Dest 和 Source Register 为 8-bit | 实为 **4-bit**（编码中各占 4 位，合占 1 字节） |
| 附录 C, p767 | `make $(getconf` | `make -j $(getconf`（缺 `-j`） |

### 1.4 原书"计划中"已实现的功能

| 书中位置 | 计划内容 | 现状 |
|---------|---------|------|
| 5.5.1, p173 | `bpf_probe_read_kernel()` / `bpf_probe_read_user()` | Linux 5.5 已合并 |
| 5.15.2, p174 | bpftrace `signal()` 函数 | 已实现 |
| 5.15.2, p175 | bpftrace `override_return()` 函数 | 已实现 |
| 5.10.3, p155 | bpftrace `if else` 支持 | 已实现 |
| 5.10.4, p155 | bpftrace `while()` 循环 | 已实现，后续又加了 `for` 循环 |

---

## 二、2019 年后 BPF 内核特性演进

原书写作时主流内核为 4.15–5.0。以下按内核版本梳理关键 BPF 特性：

### 2.1 内核版本时间线

| 内核 | 发布 | BPF 关键特性 | HFT 关联 |
|------|------|-------------|----------|
| 5.5 | 2020-01 | `bpf_probe_read_kernel()` / `bpf_probe_read_user()` 分离 | 替代旧 `bpf_probe_read()`，内核/用户态读取更安全 |
| 5.7 | 2020-06 | BTF 支持 dump 到 `/sys/kernel/btf/vmlinux`；struct_ops | CO-RE 实用化基础 |
| 5.8 | 2020-08 | `bpf_iter`（可迭代 BPF 程序）；trampoline links | 替代 perf_event 附着，更低开销 |
| 5.9 | 2020-10 | `bpf_link` 对象（持久化附着） | 工具不再需要常驻进程 |
| 5.12 | 2021-03 | BPF 程序睡眠支持（`bpf_copy_from_user()` 等） | 可在探针中安全做用户态拷贝 |
| 5.13 | 2021-06 | bpf_get_socket_cookie；kfunc 机制萌芽 | — |
| 5.14 | 2021-08 | bpf_timer；`bpf_loop()` helper | 有界循环不再是梦 |
| 5.15 | 2021-10 | bpf_get_branch_snapshot（LBR 快照） | **HFT：捕获分支预测失败路径** |
| 6.0 | 2022-10 | bpf_loop() 正式可用；BPF_MAP_TYPE_USER_RINGBUF | — |
| 6.2 | 2023-02 | `bpf_arena` 初步讨论 | — |
| 6.7 | 2024-01 | 异常处理（`bpf_throw()`）；Netkit（替代 veth pair） | **容器网络性能接近宿主机** |
| 6.8 | 2024-03 | 文件验证（LSM + fsverity）；验证器改善 | — |
| 6.9 | 2024-05 | **BPF Arena**（稀疏共享内存）；**BPF Token**（委托给非特权容器） | Arena：高吞吐内核↔用户态数据交换；Token：容器内 eBPF |
| 6.10 | 2024-07 | 工作队列（异步执行）；**KPROBE 会话**（入口+返回点同时挂载）；禁抢占 | **KPROBE 会话：函数耗时测量一步到位** |
| 6.11 | 2024-09 | 新 uretprobe 系统调用（**x86_64 快 10–30%**）；BTF 导出 kfunc 原型 | **uretprobe 提速直接利好 HFT 用户态插桩** |
| 6.12 | 2024-11 | **PREEMPT_RT 合入主线**；**sched_ext**（BPF 可编程调度器）；EEVDF 完成 | **🔴 HFT 里程碑：主线实时内核 + BPF 自定义调度策略** |

### 2.2 重点特性详解

#### PREEMPT_RT（Linux 6.12）

经过 20 年开发，PREEMPT_RT 终于合入主线。内核代码路径（printk 最后一块）全部可抢占，提供确定性低延迟。对 HFT 意义重大——不再需要打实时补丁树。

**架构支持：** x86_64、ARM64、RISC-V。

#### sched_ext（Linux 6.12）

BPF 可编程调度器类。开发者可以用 BPF 程序定义任务调度算法，无需修改内核源码。

- **文档：** `Documentation/scheduler/sched-ext.rst`
- **已有实践：** 游戏场景帧率优化调度器、HFT 场景 CPU 亲和性调度
- **HFT 场景：** 可编写 BPF 调度器把交易线程绑定到独占核，避免调度器抖动

#### BPF Arena（Linux 6.9）

eBPF 程序与用户空间之间的稀疏共享内存区域。比 ringbuf/perf buffer 更灵活，支持随机访问。

- 适合高吞吐数据交换（如批量包捕获、大块数据传输）
- libbpf 1.4+ 支持 `__arena` 全局变量

#### BPF Token（Linux 6.9）

允许特权守护进程将 BPF 子系统的部分功能委托给受信任的非特权应用。容器化 eBPF 应用可在用户命名空间中运行。

#### KPROBE 会话（Linux 6.10）

一个 BPF 程序同时挂载到 kprobe 入口和 kretprobe 返回点。原书第 18 章提到的"双探针配对计时"模式现在可以一步实现。

#### uretprobe 提速（Linux 6.11）

新的 uretprobe 系统调用在 x86_64 上提速 10–30%。原书附录 C 中 uprobes 被标注为"最贵探针"（1931ns），此优化直接降低用户态返回探针开销。

---

## 三、libbpf 演进（CO-RE 生态）

### 3.1 从 BCC 到 libbpf 的范式迁移

原书以 BCC Python 脚本为主要开发方式。2019 年后生态明确转向 **libbpf + CO-RE**：

| 维度 | BCC Python（原书方式） | libbpf CO-RE（现代方式） |
|------|----------------------|------------------------|
| 编译 | 运行时 LLVM/Clang 编译（160ms+ 启动） | 预编译 BPF 字节码（<1ms 加载） |
| 依赖 | libbcc + LLVM + Clang（80MB+） | 仅 libbpf（<1MB） |
| 可移植性 | 需目标机器有内核头文件 | BTF + CO-RE 重定位，一次编译到处运行 |
| 语言 | Python（含 BPF C 内联） | C/C++（BPF C + 用户态 C） |
| 状态 | **BCC Python 接口已标记 deprecated** | 活跃开发 |

> **原书附录 C 的 BCC Python 教程仍可作为学习材料**，但生产环境应迁移到 libbpf。BCC 仓库内的 `libbpf-tools/` 目录提供了同名工具的 C 版本。

### 3.2 libbpf 版本时间线

| 版本 | 发布 | 关键特性 |
|------|------|---------|
| 1.0 | 2022-09 | 首个稳定版；移除旧 API；BPF skeleton 成熟 |
| 1.1 | 2023-01 | uprobe_multi link 支持 |
| 1.2 | 2023-05 | bpf_loop() 支持 |
| 1.3 | 2023-10 | libbpf-cargo（Rust 绑定）稳定 |
| 1.4 | 2024-04 | BPF token、arena maps、raw tracepoint cookies、`__arg_ctx` 标注 |
| 1.5 | 2024-10 | BPF object 预处理步骤；改进错误报告 |
| 1.6 | 2025-07 | BPF token 附着支持；multi-uprobe session；blazesym 符号化 |

### 3.3 CO-RE（Compile Once – Run Everywhere）

利用 BTF 类型信息在加载时重定位结构体字段偏移。一个编译好的 BPF 程序可跨不同内核版本运行，只要目标内核提供 BTF。

- 2024 年后主流发行版（Ubuntu、Fedora、RHEL）默认启用 BTF
- `bpftool btf dump file /sys/kernel/btf/vmlinux format c` 可导出内核 BTF
- libbpf 提供 `bpf_core_read()` 宏安全读取跨版本结构体字段

---

## 四、bpftrace 演进（0.20 → 0.25）

原书写作时 bpftrace 版本约 0.9。截至 2025 年已发布到 0.25，语言能力大幅增强。

### 4.1 版本时间线与关键特性

| 版本 | 发布 | 关键新增 | 破坏性变更 |
|------|------|---------|-----------|
| 0.20 | 2024-01 | `log2` 直方图可调粒度；`jiffies` 内建；`uprobe_multi` link；`fentry`/`fexit` 别名 `kfunc`/`kretfunc`；`config` 块语法；`kprobe:module:function` | 移除 snapcraft |
| 0.21 | 2024-06 | `for-each` 循环遍历 map；`lazy_symbolication` 配置；uprobe 可挂载内联函数；`count/sum/min/max` 内核态读取；LLVM 18 支持 | 废弃 `sarg` 内建 |
| 0.22 | 2025-01 | `pid`/`tid` 返回 uint32（非 uint64）；块级作用域变量；`let` 变量声明；`has_key()` 函数；元组做 map key；`for` 循环支持多探针变量共享；C++ 类继承解析；`--dry-run` | 移除多 key `delete` 语法；`SIGUSR1` 不再打印 map |
| 0.23 | 2025-03 | `offsetof()` 支持子字段；指针可做条件判断；`len()` 支持 `ustack`/`kstack`；blazesym 内核符号化；LLVM 20 支持；enum 类型转换 | 移除 `-kk` 选项；移除 LLVM 14/15 支持 |
| 0.24 | 2025-09 | **🔴 宏（hygienic macros）**；命名参数 `getopt()`；`tseries` 时间序列图；map 声明（`lruhash(100)`）；`for ($i : 0..ncpus)` 范围循环；布尔值 `true`/`false`；持续时间字面量 `1s`；**移除大部分 DWARF 支持**（仅保留 uprobe 参数解析）；rawtracepoint 要求 BTF；`BPF_MAP_TYPE_RINGBUF` 必须可用；`strcontains`/`has_key` 返回布尔值；`ncpus` 内建；`usermode` 内建 | 探针附着失败默认退出程序（可配置 `missing_probes`）；DWARF 大幅移除 |
| 0.25 | 2025-11 | **🔴 Record 类型**（命名字段元组）；`import` 语句（实验性）；`--fmt` 脚本格式化；`pcomm` 内建；`build_id` 栈模式；`.` 自动解引用（替代 `->`）；`find()` 查找 map 值；`syscall_name()` 转换系统调用号；多 begin/end 探针 | `exit()` 不再允许在循环内；移除 `sarg`；tracepoint `args` 要求 BTF；移除 `BPFTRACE_DEBUG_OUTPUT` |

### 4.2 原书语法 vs 现代语法对照

| 原书写法（2019） | 现代写法（0.25） | 说明 |
|-----------------|-----------------|------|
| `$x = "hello"` 在 if 块中赋值 | 需先 `let $x;` 或在块外初始化 | 块级作用域 |
| `delete(@b[1], @b[2], @b[3])` | `delete(@b, 1); delete(@b, 2); delete(@b, 3)` | 多 key delete 已移除 |
| `->` 成员访问 | `.` 自动解引用（推荐） | 0.25 新增 |
| 无循环（仅 `while`） | `for ($i : 0..ncpus)` | 0.24 范围循环 |
| 位置参数 `$1` | `getopt("name", "default")` | 0.24 命名参数 |
| 手动写 BPF C 代码 | 宏 + `import` | 0.24/0.25 代码复用 |
| `args` 在 tracepoint 中 | 需要内核 BTF | 0.25 强制要求 |

### 4.3 bpftrace 1.0 路线图

bpftrace 官方发布了 [The Path to 1.0](https://bpftrace.org) 博文，主要目标：
- 类型系统全面 BTF 化
- 编译管线重构
- 标准库建立
- 稳定性保证

---

## 五、BCC 演进

### 5.1 libbpf-tools 迁移

BCC 仓库内 `libbpf-tools/` 目录逐步将 Python 版工具重写为 C + libbpf 版本：

| 工具 | 状态 | 改进 |
|------|------|------|
| opensnoop | 已迁移 | 新增字段；支持 PID 命名空间 |
| biolatency | 已迁移 | 使用 tracepoint 替代 kprobe |
| biotop | 已迁移 | `dump_hash` 批量读取 |
| hardirqs/softirqs | 已迁移 | 对数计算修复；CPU 列 |
| memleak | 已迁移 | off-by-one 修复 |
| klockstat | 已迁移 | 更好的栈摘要 |
| sigsnoop | 已迁移 | 支持实时信号和线程 comm |
| statsnoop | 已迁移 | 支持更多系统调用 |
| profile | 已迁移 | 优先使用 cpu-cycles 硬件事件 |
| mountsnoop | 已迁移 | 支持 fsopen/fsconfig/fsmount/move_mount |
| tcpdrop | 已迁移 | 支持 TCP drop reasons |

### 5.2 BCC 版本时间线

| 版本 | 发布 | 关键变化 |
|------|------|---------|
| 0.30 | 2024-03 | — |
| 0.31 | 2024-07 | — |
| 0.32 | 2024-11 | — |
| 0.33 | 2025-01 | 支持 kernel 6.12；新增 `numasched` 工具 |
| 0.34 | 2025-04 | 支持 kernel 6.13；`statsnoop` 显示系统调用名；`readahead` 修复 5.16+ 页计数；`tcpdrop` 支持 drop reasons；`profile` 优先用硬件事件 |
| 0.35 | 2025-05 | 支持 kernel 6.14；新增 `mptcp` 工具；`biosnoop` pattern 选项修复 |
| 0.36 | 2026-01 | — |

### 5.3 BCC Python 接口弃用

原书附录 C 基于 BCC Python 接口（`from bcc import BPF`）。该接口已标记为 deprecated：

- **推荐替代：** libbpf-tools（C + libbpf）或 bpftrace
- **仍可使用：** BCC Python 接口仍可工作，但不接受新特性
- **学习价值：** 原书附录 C 的教学价值仍在——理解 BPF 程序结构、map 类型、perf buffer 机制

---

## 六、HFT 场景 BPF 应用

### 6.1 行业采纳

- **Goldman Sachs**：生产工程岗位明确要求 eBPF 经验，需要"eBPF 和追踪用于生产可观测性"以及"RDMA 或内核网络内部知识"
- **Linux Foundation 2026 年 2 月报告**：eBPF 是"在生产环境中应用自定义网络、安全、性能或可观测性功能的主要载体"
- **IEEE ISPASS 2024 论文**（Rezvani）：专门评估 eBPF 在延迟敏感应用中的内核级可观测性，证明系统调用监控开销极低

### 6.2 HFT 关键 BPF 场景

| 场景 | BPF 技术 | HFT 价值 |
|------|---------|----------|
| **TCP 重传监控** | tracepoint `tcp:tcp_retransmit_skb`；Tetragon 监控模式 | 交易所连接丢包实时发现 |
| **系统调用延迟** | kprobe/kretprobe 配对计时 | 定位 write/sendto 等关键路径延迟 |
| **上下文切换频率** | tracepoint `sched:sched_switch` | 识别 CPU 缓存失效导致的性能悬崖 |
| **内核调度延迟** | `runqlat`；`cpuwalk` | 订单处理峰值吞吐量优化 |
| **XDP 网络加速** | XDP（eXpress Data Path） | 订单报文在网卡驱动层直接处理，绕过内核协议栈 |
| **BPF MAP 零拷贝** | `BPF_MAP_TYPE_PERCPU_ARRAY` | 用户态与内核态共享内存，避免拷贝 |
| **sched_ext 自定义调度** | BPF 调度器程序（Linux 6.12+） | 交易线程独占 CPU 核，消除调度抖动 |
| **uretprobe 用户态插桩** | uprobe/uretprobe（Linux 6.11 提速） | 监控交易引擎关键函数耗时 |

### 6.3 HFT 部署准则

1. **探针中绝不阻塞**——不发起网络请求、不写磁盘、不获取锁。收集数据立即返回
2. **优先 tracepoint 而非 kprobe**——稳定 API、更低开销
3. **限制事件范围**——只追踪特定系统调用、特定连接、特定 PID
4. **使用 per-CPU 数据结构**——避免锁竞争
5. **先 staging 后生产**——eBPF 行为可能因内核版本而异
6. **Tetragon 优先监控模式**——交易环境中避免启用执行功能

### 6.4 推荐工具组合

| 用途 | 工具 | 说明 |
|------|------|------|
| 快速诊断 | `bpftrace` 一行命令 | 临时探查系统调用/内核函数 |
| 长期监控 | Tetragon（监控模式） | TCP 重传追踪，无需修改应用 |
| 性能剖析 | `profile.bt`（99Hz 采样） | 低开销火焰图 |
| 网络延迟 | `tcplife.bt` / `tcpdrop.bt` | TCP 会话生命周期和丢包 |
| 系统调用 | `execsnoop` / `opensnoop` | 进程创建和文件访问 |
| IO 延迟 | `biolatency.bt` | 块设备 IO 延迟分布 |

---

## 七、其他生态工具更新（原书第 17 章补充）

### 7.1 原书提及工具的现状

| 工具 | 原书状态 | 现状 |
|------|---------|------|
| Cilium | 网络方案 | 已扩展为 Cilium + Hubble（可观测性）+ Tetragon（安全） |
| kubectl-trace | iovisor 实验项目 | 仍为实验状态，社区活跃度低 |
| ply | 轻量级追踪器 | 仍维护，适合嵌入式场景 |
| Pixie | 未提及（书后出现） | Kubernetes 原生可观测性，基于 eBPF，无需修改代码 |
| Inspektor Gadget | 未提及 | Kubernetes 集群调试工具集 |
| bpftrace | 0.9 版本 | 0.25 版本，向 1.0 迈进 |

### 7.2 新出现的工具（原书未覆盖）

| 工具 | 说明 | HFT 关联 |
|------|------|----------|
| **bpftime** | 用户态 eBPF 运行时，快速 uprobe/syscall hook | 用户态 BPF 程序无需内核介入 |
| **retsnoop** | BPF 大规模内核函数追踪（Andrii Nakryiko） | 内核问题快速定位 |
| **blazesym** | 高性能符号化库 | 替代旧版 addr2line，bpftrace 已集成 |
| **OBI**（OpenTelemetry eBPF Instrumentation） | 自动捕获所有应用指标和追踪 | HTTP/gRPC/SQL/Redis/MongoDB 协议级监控 |
| **XDP2** | 统一软硬件数据面编程模型 | SmartNIC 上运行 BPF |

---

## 八、原书附录 D/E 补充说明

### 8.1 附录 D（C 语言 BPF）的现代替代

原书附录 D 介绍了 `bpf_load.h` 和手工构造 BPF 指令的方式。现代开发应使用：

- **libbpf + BPF skeleton**：自动加载/附着，类型安全
- **BTF CO-RE**：跨内核可移植
- **bpftool gen skeleton**：生成 skeleton 头文件
- **`#define _(P)` 宏**：已被 `bpf_core_read()` 替代

### 8.2 附录 E（BPF 指令）验证工具

原书提到的 `bpf(2)` 系统调用和手工字节码构造，现在可用 `bpftool` 可视化验证：

```bash
# 查看已加载 BPF 程序的指令
bpftool prog dump xlated id <ID>

# 查看对应的 JIT 机器码
bpftool prog dump jited id <ID>

# 验证 BPF 指令编码
# BPF_MOV64_IMM(BPF_REG_1, 0xa21) 应展开为:
# opcode=0xb7, dst=0x01, src=0x00, off=0x0000, imm=0x00000a21
```

---

## 九、与原书的对照阅读建议

| 原书章节 | 网络补充重点 |
|---------|-------------|
| 第 2 章 技术背景 | 补读：BTF、CO-RE 概念（本文第二节） |
| 第 4 章 BCC | 补读：libbpf-tools 迁移、Python 弃用（本文第五节） |
| 第 5 章 bpftrace | 补读：0.20→0.25 语法变化（本文第四节） |
| 第 6 章 CPU | 补读：sched_ext、PREEMPT_RT（本文 2.2） |
| 第 10 章 网络 | 补读：XDP2、OBI（本文 7.2） |
| 第 14 章 内核 | 补读：bpf_iter、bpf_link、kfunc（本文第二节） |
| 第 16 章 虚拟化 | 补读：Nitro Enclaves、KVM 改进（搜原文） |
| 第 17 章 其他工具 | 补读：Cilium/Hubble/Tetragon 演化（本文 7.1） |
| 第 18 章 建议与技巧 | 补读：bpftrace 破坏性变更迁移（本文 4.2） |
| 附录 C BCC 开发 | 补读：libbpf skeleton 工作流（本文第三节） |
| 附录 D C BPF | 补读：libbpf 替代 bpf_load（本文 8.1） |
| 附录 E BPF 指令 | 补读：bpftool 验证（本文 8.2） |

---

## 参考链接

- [Brendan Gregg 书籍页面](https://www.brendangregg.com/bpf-performance-tools-book.html) — 官方资源、勘误、更新
- [bpf-perf-tools-book GitHub](https://github.com/brendangregg/bpf-perf-tools-book) — 工具源码
- [bpftrace 官网](https://bpftrace.org) — 文档、发布说明
- [bpftrace 迁移指南](https://github.com/bpftrace/bpftrace/blob/master/docs/migration_guide.md) — 破坏性变更
- [libbpf GitHub](https://github.com/libbpf/libbpf) — 源码和发布
- [KernelNewbies Linux 6.12](https://kernelnewbies.org/Linux_6.12) — 内核特性详解
- [eBPF 2024 年度总结](http://uaxe.github.io/geektime-docs/) — 中文 eBPF 生态总结
- [eBPF for Trading Systems](https://cloudlogic.dev/2026/07/12/ebpf-for-trading-systems-kernel-level-observability-without-the-performance-tax/) — HFT 场景 BPF 应用
