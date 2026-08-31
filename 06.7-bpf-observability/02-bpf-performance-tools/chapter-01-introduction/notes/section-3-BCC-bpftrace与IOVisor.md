# 1.3 BCC、bpftrace 和 IOVisor

> 底本：《BPF之巅》中文版 1.3 节（PDF p43–44）

## 为什么需要前端

直接写 BPF 指令非常烦琐，所以发展出高级语言前端。跟踪领域的两大主流：

| | **BCC** | **bpftrace** |
|---|---|---|
| 定位 | 最早的 BPF 跟踪高级框架 | 新近出现的专用高级语言 |
| 内核侧 | C 语言环境 | 自有 DSL（基于 libbcc/libbpf 构建） |
| 用户侧 | Python / Lua / C++ | 同一语言 |
| 自带工具 | **70+ 个** | **20+ 个**（.bt） |
| 擅长 | 复杂脚本、后台进程、调用其他库（如 Python argparse 做精细命令行参数） | **单行程序、短小脚本**（源码极简，本书可直接贴源码讲原理） |

两者**互补**，不互斥。

## BCC 的架构代价：运行时编译

BCC 的内核侧是 C，意味着**每次工具启动都要现场把 C 源码编译成 BPF 字节码**（内置 Clang/LLVM）：

- 优点：工具源码就是普通 C，开发者可以在程序里用宏和循环逻辑（bpftrace DSL 做不到的表达力）
- 代价一：**目标机必须装内核头文件**（BCC 编译时要 include 对应版本的内核结构定义）——"每台机器都要装一大坨开发包"
- 代价二：**结构体布局绑定编译时的内核版本**——工具与内核版本错位时可能编不过，或更糟：编过了但字段偏移错（CO-RE/BTF 就是为了解决这个，见下）
- 代价三：LLVM 运行时依赖让 BCC 工具二进制约几十 MB，最小化安装的交易机/嵌入式设备放不下

这就是 bpftrace 也长不大、libbpf+CO-RE 单二进制最终胜出的结构性原因（[Learning eBPF Ch5](../../../01-learning-ebpf/chapter-05-core-btf-libbpf/)：BCC 之弊与 CO-RE 要素）。

## 库的血统

- BCC 是 **libbcc 和 libbpf 库的前身**（最早的 libbpf 由 Wang Nan 为 perf 开发，如今 libbpf 已并入内核代码树）
- bpftrace 基于 libbcc 和 libbpf 构建
- libbpf 进入内核树（tools/lib/bpf）是个分水岭：它让"用户态加载器"与内核 ABI 的演进同步维护，CO-RE 重定位、BPF link 等 API 都以 libbpf 为参考实现——**新工具链开发默认直接用 libbpf，不再经 libbcc**

## 周边项目

- **ply**：开发中的轻量前端，依赖最小化，适合**嵌入式 Linux**；数十个 bpftrace 工具转成 ply 语法即可用。本书选 bpftrace 是因为它更成熟、特性齐全
- **IOVisor**：BCC 和 bpftrace 都不属于内核代码仓库，托管在 Linux 基金会的 IOVisor 项目（GitHub）

## 术语约定

本书说"**BPF 跟踪**"时，同时涵盖 BCC 和 bpftrace 两个版本的工具。

---

### HFT 关联

- 现场排障优先级：**BCC 现成工具 → bpftrace 单行/短脚本 → 定制 BCC**。bpftrace 是"问答式"探针，10 秒内回答一个假设；BCC 工具适合挂后台长跑收集
- 交易机最小化安装的场景（发行版不带 BCC 的重型依赖链）→ 对应 ply 的嵌入式定位思路；现代替代是 libbpf + CO-RE 单二进制（见 [learning-ebpf Ch5](../../../01-learning-ebpf/chapter-05-core-btf-libbpf/)）——静态编译、无运行时编译、跨内核版本可移植，正是交易机部署观测探针的理想形态
- 版本错位风险要写进运维 runbook：**BCC 工具跟着内核升级走**（重装 headers + 重编译），CO-RE 工具跟着 BTF 走（`/sys/kernel/btf/vmlinux` 存在即可）——两类工具的"升级检查项"不同，混在一张表里会漏检

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 为什么要求目标机安装内核头文件？这个约束的根源是什么？**

   <details><summary>参考答案</summary>

   BCC 在工具启动时现场编译 C 内核态程序，C 源码要 include 内核结构定义（task_struct 等布局）才能编译通过。根源：BCC 不做重定位——结构体偏移在编译期烧死，必须有编译期对应的头文件。CO-RE 用 BTF + 重定位把这一步移到加载期，才解除了这个约束。

   </details>

2. **bpftrace 和 BCC 各自的"甜点区"是什么？给出一个具体分工例子。**

   <details><summary>参考答案</summary>

   bpftrace：单行/短脚本的快速问答（"现在谁在打开这个文件？"）。BCC：需要命令行参数、后台长跑、组合多事件源的成熟工具（opensnoop -p PID -x 挂着收证据）。例子：先用 bpftrace 单行确认假设存在，再让 BCC 工具带过滤参数长跑收集完整证据链。

   </details>

</details>
