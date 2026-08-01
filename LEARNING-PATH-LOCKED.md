# 锁定学习路线（定稿）

> **目标：** 嵌入式 Linux 底层开发 + HFT 低延迟。  
> **结论：** 书单深度够、广度闭环；**不要再扩书**。成败在于 **自底向上顺序** + **动手 Demo**。  
> **重要：** **文件夹编号 = 学习顺序**（`00` → `23`）。书名只出现在各模块 README / `refs/`。

---

## 递进主轴

```text
硬件底层 → 编程语言 → Linux 系统 → 驱动/设备树 → 嵌入式工程 → 网络栈 → 性能工具 → HFT 上层业务
```

---

## 模块一览（编号 = 读序）

| # | 文件夹 | 定位 |
|---|--------|------|
| **00** | [00-digital-logic-cpu](./00-digital-logic-cpu/) | 硬件底层：组合/时序/CPU 词汇 |
| **01** | [01-c-language](./01-c-language/) | C / 指针 / GNU-C |
| **02** | [02-computer-systems](./02-computer-systems/) | 程序=机器：栈/缓存/VM/并发 |
| **03** | [03-computer-architecture](./03-computer-architecture/) | 体系结构加深（可后读） |
| **04** | [04-linux-userspace-api](./04-linux-userspace-api/) | 用户态系统编程 |
| **05** | [05-os-from-scratch](./05-os-from-scratch/) | 自制 OS 动手 |
| **06** | [06-cpp](./06-cpp/) | C++ |
| **07** | [07-linux-kernel](./07-linux-kernel/) | 内核入门 |
| **08** | [08-linux-kernel-deep](./08-linux-kernel-deep/) | 内核深度（拓展） |
| **09** | [09-linux-mm](./09-linux-mm/) | 内核内存管理 |
| **10** | [10-arm-architecture](./10-arm-architecture/) | ARM / AArch64 |
| **11** | [11-embedded-boot-build](./11-embedded-boot-build/) | U-Boot / 内核构建 / rootfs |
| **12** | [12-device-drivers-dt](./12-device-drivers-dt/) | 驱动 + 设备树 |
| **13** | [13-embedded-projects](./13-embedded-projects/) | 板级 / 无人机 / 网关实战 |
| **14** | [14-motion-control](./14-motion-control/) | PID / 姿态 / 飞控（兴趣） |
| **15** | [15-network-sockets](./15-network-sockets/) | Socket 编程 |
| **16** | [16-tcpip-protocols](./16-tcpip-protocols/) | TCP/IP 协议 |
| **17** | [17-kernel-networking](./17-kernel-networking/) | 内核网络栈 |
| **18** | [18-dpdk](./18-dpdk/) | 用户态高速网络 |
| **19** | [19-systems-performance](./19-systems-performance/) | 系统性能方法论 |
| **20** | [20-bpf-observability](./20-bpf-observability/) | BPF / 可观测 |
| **21** | [21-hft-engineering](./21-hft-engineering/) | HFT 工程实践 |
| **22** | [22-rust-quant](./22-rust-quant/) | Rust 量化（拓展） |
| **23** | [23-markets-microstructure](./23-markets-microstructure/) | 交易 / 微观结构（业务） |

---

## Phase 顺序

```
Phase1  00 数字逻辑/CPU（当前；未完成前不正式开下一 Phase）
   ↓
Phase2  01 C → 02 计算机系统
   ↓
Phase3  04 用户态 API →（穿插 05 自制 OS / 06 C++）
   ↓
Phase4  07 内核 + 同步 09 MM（08 内核深度可后补）
   ↓
Phase5  分叉并行
        A 嵌入式: 10 → 11 → 12 → 13（14 兴趣）
        B HFT:    15 → 16 → 17 → 18 → 19 → 20 → 21
   ↓
Phase6  拓展: 03 · 08 · 22 · 23 ·（兴趣）14
```

### Phase 细则

| Phase | 内容 | 过关感 |
|-------|------|--------|
| **1** | `00` 数字逻辑/CPU（黑盒语义为主） | setup/hold、寄存器与 FIFO；不纠结门级 |
| **2** | `01` C → `02` 计算机系统 | 指针/内存过关；流水线、Cache、VM、并发能讲通 |
| **3** | `04` → 穿插 `05`/`06` | 进程/线程/信号/`mmap`/`epoll`；能写小 Demo |
| **4** | `07` · `09` | 调度、内存、同步入门地图清晰 |
| **5A** | `10`–`13` | 启动链、设备树、简单驱动、板级闭环 |
| **5B** | `15`–`21` | Socket → 协议 → 内核网 → DPDK → 观测 → HFT |
| **6** | 拓展书/业务 | 主线闭环后再加 |

---

## 深度约束（已定）

- `00`：组合/时序取黑盒语义；门级/Verilog 不主攻 → 见 `00-…/学习深度_*.md`
- `02`：流水线/缓存/VM 为主粮；Ch4 是 Y86+HCL，不是 Verilog

---

## 当前状态

- **正在：** Phase1 · `00-digital-logic-cpu`
- **下一站：** Phase2 · `01-c-language` → `02-computer-systems`
- **暂不新开：** `07`/`17`/`18`/`21` 等（除非做极小对照实验）

---

## 必须警惕

1. **禁止乱跳**：未完成 Phase1/2 不要冲内核、DPDK、HFT。  
2. **时间不均分**：电机、Rust 量化、体系结构加深前期少投入。  
3. **必须动手**：无锁队列、绑核、大页、简易 UDP；嵌入式侧编译内核、设备树调试。  
4. **少开并行文件夹**：优先啃透当前 Phase。
