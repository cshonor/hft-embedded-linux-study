# 《eBPF 学习手册》读书笔记（导读索引）

> **英文：** *Learning eBPF*  
> **作者：** Liz Rice（eBPF 基金会核心人物，Isovalent/Netlify CTO）  
> **出版：** 2023（O'Reilly）· **中文版：** 2023-04 中国电力出版社 ISBN 9787519889883  
> **内核：** 5.15（Ubuntu 22.04 测试基准）  
> **代码：** [github.com/lizrice/learning-ebpf](https://github.com/lizrice/learning-ebpf)（含 libbpf 子模块 + Lima VM 配置）  
> **本地电子书：** LEARNING-EBPF-BILINGUAL.pdf（GPT 中英双语版，551 页）— 版权原因不入库，存 `~/Desktop/hft-local-books/`  
> **定位：** [06.7-bpf-observability/](../) 模块的**现代原理书** — 补 [BPF Performance Tools](../02-bpf-performance-tools/BPF-PERFORMANCE-TOOLS-EVAL.md) 缺的 libbpf/CO-RE/验证器/程序类型  
> **姊妹对比：** [EBPF-BOOKS-COMPARISON.md](../EBPF-BOOKS-COMPARISON.md)

---

## 章节笔记（已全部完成，基于 GPT 双语 PDF 逐章精读）

全部 11 章笔记位于本目录，与 [BPF Performance Tools](../02-bpf-performance-tools/) 同一套目录约定：

```
chapter-XX-english-slug/
├── README.md      ← 章导读（原书信息、HFT 标注、本章目标、小节索引、交叉引用）
└── notes/         ← 按主题合并的笔记（X.Y_主题.md：概念详解 / 坑点清单 / HFT 关联 / 自测题）
```

| 章 | 笔记 | 核心内容 | HFT | 读法 |
|----|------|------|-----|------|
| 1 | [chapter-01-what-is-ebpf](./chapter-01-what-is-ebpf/) | eBPF 起源（BPF→eBPF）、内核模块对比、动态加载、云原生演进 | 🟡 | 选读 — 概念入门，已懂可跳 |
| 2 | [chapter-02-hello-world](./chapter-02-hello-world/) | BCC 框架 Hello World、BPF map（hash/perf/ringbuf）、尾调用 | 🔴 | 精读 — map 是所有观测数据传递的基础 |
| 3 | [chapter-03-anatomy-of-ebpf-program](./chapter-03-anatomy-of-ebpf-program/) | eBPF 虚拟机/寄存器/指令、XDP 示例、字节码→机器码、BPF-to-BPF 调用 | 🔴 | 精读 — 理解程序在内核里怎么跑 |
| 4 | [chapter-04-bpf-syscall](./chapter-04-bpf-syscall/) | bpf syscall 全貌、加载程序/创建 map/操作 map、用户态↔内核态交互 | 🔴 | 精读 — 理解加载与数据通路 |
| 5 | [chapter-05-core-btf-libbpf](./chapter-05-core-btf-libbpf/) | BCC 五痛点、CO-RE 五要素、BTF/vmlinux.h、重定位、libbpf 骨架 | 🔴 | 精读 — **现代开发栈核心，BPF 之巅缺的就在这** |
| 6 | [chapter-06-verifier](./chapter-06-verifier/) | 验证算法（寄存器状态/剪枝）、六类典型拒绝、循环演进、bpf_loop | 🔴 | 精读 — 写程序必过的一关 |
| 7 | [chapter-07-program-attachment-types](./chapter-07-program-attachment-types/) | 决定链、五种 execve 挂法、fentry/fexit、uprobe、LSM、约 30 种程序类型 | 🔴 | 精读 — 选对附加点是观测设计的关键 |
| 8 | [chapter-08-networking](./chapter-08-networking/) | XDP 五返回码/包解析/负载均衡、TC、uprobe 钩 SSL 明文、Cilium/K8s | 🔴 | 精读 — HFT 网络延迟观测核心 |
| 9 | [chapter-09-security](./chapter-09-security/) | seccomp、TOCTOU、BPF LSM、Tetragon、bpf_send_signal 同步阻断 | 🟡 | 选读 — 交易机加固方向值得看 |
| 10 | [chapter-10-programming](./chapter-10-programming/) | bpftrace、BCC、libbpf、cilium/ebpf(bpf2go)、libbpfgo、Aya/Rust 选型 | 🟡 | 选读 — Rust eBPF 可对接 [18-rust-quant](../../18-rust-quant/) |
| 11 | [chapter-11-future](./chapter-11-future/) | eBPF 基金会、Windows 版架构、签名/指针/内存分配在研方向 | ⚪ | 跳过 — 了解即可 |

### 真机实验佐证（2026-08 起逐步补强）

理论笔记之外，配套真机实验在 [cshonor/ebpf-gate](https://github.com/cshonor/ebpf-gate)（树莓派 5 / aarch64 / 6.18 内核 / clang 19 / bpftrace 0.23），一手实验结论已回灌对应章节：

| 实验 | 回灌位置 |
|---|---|
| lab02：无 BTF 内核上 tracepoint 追踪 openat（手工 format 结构体） | Ch5 §3.5、Ch5 坑点 9-10 |
| aarch64 交叉编译 `asm/types.h` 坑与修复 | Ch3 §3 |
| `invalid bpf_context access off=4` 真实 verifier 拒绝日志 | Ch6 §3.6 |
| 用户态指针必须 probe_read、printk %s 收指针 | Ch2 坑点 7-8 |
| lab04：strace 拆解 libbpf 的 50 个 bpf() 调用（探测 vs 真活儿；BTF_LOAD 在无内核 BTF 机器上成功；BPF_LINK_CREATE 挂载；memfd placeholder fd 槽位） | Ch4 §2 |
| lab05：verifier 六类拒绝实验集——三大"洗白"发现（未初始化栈读被 6.18 特权模式放行 commit 6715df8d5d24；ARRAY+常量 key 洗白 NULL 检查；unreachable 指令只能内联汇编注入） | Ch6 §5 |
| lab06：kprobe 挂 do_sys_openat2 跑通（do_sys_open 实测 0 次；6.18 PARM2 直接是用户态 char*）；uprobe 勘察：内核未编 CONFIG_UPROBE_EVENTS | Ch7 §2、Ch8 §4.4 |
| lab07：XDP 挂 lo 跑通（UDP 9999 网卡层 DROP）；SOCKET_FILTER 禁碰 data/data_end（改 protocol 字段过滤） | Ch8 §1.1 |
| lab08：seccomp classic BPF 沙箱跑通（ERRNO+KILL 双版本）；LSM 勘察（bpf 不在 LSM 链） | Ch9 §4 |

---

## 一句话结论

**现代 eBPF 入门首选** — 从零理解 eBPF 全貌，覆盖 libbpf + CO-RE 现代开发栈。  
读法：**先本书建原理框架 → 再 [BPF Performance Tools](../02-bpf-performance-tools/BPF-PERFORMANCE-TOOLS-EVAL.md) 练工具**。

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

[BPF Performance Tools](../02-bpf-performance-tools/BPF-PERFORMANCE-TOOLS-EVAL.md)（2019）停在 BCC Python 脚本范式，缺：
- **CO-RE**（Compile Once, Run Everywhere）— 跨内核版本可移植
- **libbpf** — C 原生开发框架，替代 BCC 重编译开销
- **BTF** — BPF Type Format，CO-RE 的基础
- **验证器**机制 — 为什么程序会被拒、怎么写合规代码
- **程序/附加类型** — kprobe/uprobe/tracepoint/xdp/tc/perf_event 等完整谱系

本书 Ch5–Ch7 正好补这四块。

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
- 与 [DPDK](../../13-dpdk/) 对照：eBPF 是"内核内观测"，DPDK 是"内核旁路绕过"，两者互补

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

**优先级：** 在 [BPF Performance Tools](../02-bpf-performance-tools/BPF-PERFORMANCE-TOOLS-EVAL.md) 之后入；如果只想买一本 eBPF 书且要从零学，**先买这本**。
