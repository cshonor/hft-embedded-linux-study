# 《eBPF 学习手册》读书笔记

> **英文：** *Learning eBPF*  
> **作者：** Liz Rice（eBPF 基金会核心人物，Isovalent/Netlify CTO）  
> **出版：** 2023（O'Reilly）· **中文版：** 2023-04 中国电力出版社 ISBN 9787519889883  
> **内核：** 5.15（Ubuntu 22.04 测试基准）  
> **代码：** [github.com/lizrice/learning-ebpf](https://github.com/lizrice/learning-ebpf)（含 libbpf 子模块 + Lima VM 配置）  
> **定位：** [15-bpf-observability/](./) 模块的**现代原理补充** — 补 [BPF Performance Tools](./BPF-PERFORMANCE-TOOLS-EVAL.md) 缺的 libbpf/CO-RE/验证器/程序类型  
> **姊妹对比：** [EBPF-BOOKS-COMPARISON.md](./EBPF-BOOKS-COMPARISON.md)

---

## 一句话结论

**现代 eBPF 入门首选** — 从零理解 eBPF 全貌，覆盖 libbpf + CO-RE 现代开发栈。  
读法：**先本书建原理框架 → 再 [BPF Performance Tools](./BPF-PERFORMANCE-TOOLS-EVAL.md) 练工具**。

---

## 定位

| | |
|--|--|
| 类型 | 原理 + 编程入门；**写程序** 而非纯用工具 |
| 强项 | libbpf/CO-RE/BTF 现代栈；验证器机制；程序与附加类型；网络/安全应用；从 Hello World 到自己写 |
| 弱项 | 不如 Brendan Gregg 那本的工具谱系广度；性能分析方法论较浅；无 bpftrace 速查表 |
| 适合谁 | 内核/嵌入式/HFT 方向 — 要"自己写 eBPF 程序做定制观测"的人 |

---

## 为什么需要这本书（补 BPF 之巅的洞）

[BPF Performance Tools](./BPF-PERFORMANCE-TOOLS-EVAL.md)（2019）停在 BCC Python 脚本范式，缺：
- **CO-RE**（Compile Once, Run Everywhere）— 跨内核版本可移植
- **libbpf** — C 原生开发框架，替代 BCC 重编译开销
- **BTF** — BPF Type Format，CO-RE 的基础
- **验证器**机制 — 为什么程序会被拒、怎么写合规代码
- **程序/附加类型** — kprobe/uprobe/tracepoint/xdp/tc/perf_event 等完整谱系

本书 Ch5–Ch7 正好补这四块。

---

## 11 章笔记框架 + HFT 读法

| 章 | 标题 | 要点 | HFT | 读法 |
|----|------|------|-----|------|
| 1 | What Is eBPF and Why Is It Important? | eBPF 起源（BPF→eBPF）、内核模块对比、动态加载、云原生演进 | 🟡 | 选读 — 概念入门，已懂可跳 |
| 2 | eBPF's "Hello World" | BCC 框架 Hello World、BPF map（hash/perf/ringbuf）、尾调用 | 🔴 | 精读 — map 是所有观测数据传递的基础 |
| 3 | Anatomy of an eBPF Program | eBPF 虚拟机/寄存器/指令、XDP 示例、字节码→机器码、BPF-to-BPF 调用 | 🔴 | 精读 — 理解程序在内核里怎么跑 |
| 4 | The bpf() System Call | bpf syscall 全貌、加载程序/创建 map/操作 map、用户态↔内核态交互 | 🔴 | 精读 — 理解加载与数据通路 |
| 5 | CO-RE, BTF and Libbpf | CO-RE 原理、BTF、libbpf C 开发框架 | 🔴 | 精读 — **现代开发栈核心，BPF 之巅缺的就在这** |
| 6 | The eBPF Verifier | 验证器检查、常见拒绝原因、怎么写合规代码 | 🔴 | 精读 — 写程序必过的一关 |
| 7 | eBPF Program and Attachment Types | kprobe/uprobe/tracepoint/xdp/tc/perf_event 等完整谱系 | 🔴 | 精读 — 选对附加点是观测设计的关键 |
| 8 | eBPF for Networking | 网络栈各附加点、拦截 ping/curl、负载均衡示例 | 🔴 | 精读 — HFT 网络延迟观测核心 |
| 9 | eBPF for Security | 安全观测与策略 | 🟡 | 选读 — 除非做安全方向 |
| 10 | eBPF Programming | 各种 eBPF 库与框架（Aya/Rust 等） | 🟡 | 选读 — Rust eBPF 可对接 [20-rust-quant](../20-rust-quant/) |
| 11 | The Future Evolution of eBPF | eBPF 未来演进 | ⚪ | 跳过 — 了解即可 |

---

## HFT 核心收获

读完本书后你应能：

1. **用 libbpf + CO-RE 写定制 eBPF 观测程序** — 不依赖 BCC 重编译，跨内核可移植
2. **选对附加点** — kprobe 抓调度器、tracepoint 抓网络栈、xdp 抓入包、perf_event 抓性能计数器
3. **过验证器** — 写出内核接受的合规 eBPF 代码
4. **理解数据通路** — map（hash/ringbuf/perf）怎么把内核事件传到用户态

**HFT 落脚点：**
- 内核跟踪：kprobe `schedule()`、`__schedule()` 定位调度抖动
- 延迟观测：tracepoint 抓软中断、网卡 NAPI 轮询、TCP 栈各点时延
- 网络抓包：XDP/tc 在协议栈最早期抓包，比 tcpdump 早、损耗低
- 与 [DPDK](../13-dpdk/) 对照：eBPF 是"内核内观测"，DPDK 是"内核旁路绕过"，两者互补

---

## 代码示例

```bash
git clone --recurse-submodules https://github.com/lizrice/learning-ebpf
cd learning-ebpf
# Lima VM（推荐，预装依赖）
limactl start learning-ebpf.yaml
limactl shell learning-ebpf
sudo -s  # 加载 BPF 程序需要 root 或 CAP_BPF

# 构建 libbpf
cd libbpf/src && make install && cd ../..

# bpftool（看 jited 码需要 libbfd 支持）
git clone --recurse-submodules https://github.com/libbpf/bpftool.git
cd bpftool/src && make install
```

- 最低内核版本随章节递增，全书在 **5.15**（Ubuntu 22.04）测试通过
- 看 eBPF trace 输出：`cat /sys/kernel/debug/tracing/trace_pipe` 或 `bpftool prog tracelog`

---

## 与 BPF Performance Tools 的协同读法

```
本书 Ch1–7      建原理框架（eBPF 是什么、怎么跑、libbpf/CO-RE、验证器、程序类型）
        ↓
BPF Performance Tools   练工具谱系（BCC/bpftrace 看什么指标、火焰图、off-CPU）
        ↓
本书 Ch8 网络    回来补网络 eBPF 程序（XDP/tc 附加点），与 DPDK 对照
```

- **先原理后工具** — 先有 eBPF 全貌框架，再用 Brendan Gregg 那本的工具才不迷失
- **Ch5 CO-RE 是分水岭** — 读完这章你就理解了"为什么 BCC 在退场、libbpf 在上位"
- **Ch8 网络 + BPF 之巅 Ch10** — 两本的网络章节对照读，一个讲程序附加点、一个讲观测指标

---

## 购买建议

| 版本 | 建议 |
|------|------|
| 中文版《eBPF 学习手册》 | ✅ 入手 — 2023 翻译质量可，门槛低于英文 |
| 英文版 *Learning eBPF* | ✅ 可选 — O'Reilly 平台在线版更新及时 |
| 代码 | 免费 — GitHub 仓库，跟随章节目录 |

**优先级：** 在 [BPF Performance Tools](./BPF-PERFORMANCE-TOOLS-EVAL.md) 之后入；如果只想买一本 eBPF 书且要从零学，**先买这本**。
