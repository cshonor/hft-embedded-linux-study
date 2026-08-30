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

## 库的血统

- BCC 是 **libbcc 和 libbpf 库的前身**（最早的 libbpf 由 Wang Nan 为 perf 开发，如今 libbpf 已并入内核代码树）
- bpftrace 基于 libbcc 和 libbpf 构建

## 周边项目

- **ply**：开发中的轻量前端，依赖最小化，适合**嵌入式 Linux**；数十个 bpftrace 工具转成 ply 语法即可用。本书选 bpftrace 是因为它更成熟、特性齐全
- **IOVisor**：BCC 和 bpftrace 都不属于内核代码仓库，托管在 Linux 基金会的 IOVisor 项目（GitHub）

## 术语约定

本书说"**BPF 跟踪**"时，同时涵盖 BCC 和 bpftrace 两个版本的工具。

---

### HFT 关联

- 现场排障优先级：**BCC 现成工具 → bpftrace 单行/短脚本 → 定制 BCC**。bpftrace 是"问答式"探针，10 秒内回答一个假设；BCC 工具适合挂后台长跑收集
- 交易机最小化安装的场景（发行版不带 BCC 的重型依赖链）→ 对应 ply 的嵌入式定位思路；现代替代是 libbpf + CO-RE 单二进制（见 [learning-ebpf Ch5](../../01-learning-ebpf/chapter-05-core-btf-libbpf/)）
