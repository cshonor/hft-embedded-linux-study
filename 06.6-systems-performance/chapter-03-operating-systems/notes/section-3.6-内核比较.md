## 3.6 内核比较

> ← [3.5 其他系统模型](./section-3.5-其他系统模型.md) · [本章导读](../README.md)

---

### 哪个内核最快？

取决于 **OS 配置、工作负载、内核参与程度**。

| 内核 | 优势 | 典型场景 |
|------|------|----------|
| **Linux** | 性能改进最多、驱动/应用支持最广、社区最大 | 通用服务器、云、HFT |
| **FreeBSD** | 特定工作负载可优于 Linux | Netflix CDN（OCA 团队内核优化） |
| **其他** | Solaris、AIX 等 | 传统企业遗留系统 |

- **TOP500 超算**自 2017 年起 **100% Linux** [TOP500 17]
- Netflix：云上用 Linux，CDN 用 FreeBSD — FreeBSD 在 CDN 工作负载上 **性能更高**（2019 年 Linux 5.0 vs FreeBSD 生产对比验证）

---

### 微基准陷阱

内核性能比较常用 **微基准** — 但极易出错：

- 测某 syscall 快 10x → 生产根本不用那个 syscall
- 或用了但 flag 不同 → 微基准没测的组合
- **准确比较是高级性能工程师的工作** — 可能花数周

方法论：[Ch12.3.2 Active Benchmarking](../../chapter-12-benchmarking/)

---

### Linux 的演进与复杂性

- 第一版时 Linux **缺成熟动态追踪器** → Gregg 转全职 Linux 后帮助开发了 **BCC 和 bpftrace**（基于 eBPF）
- **OpenZFS** 现以 Linux 为 **主内核** — 提供高性能成熟文件系统
- 3.1→5.8 跨版本大量性能改进（详见 3.4.1 Linux Kernel Developments）

**但复杂性是代价：** Linux 性能特性和调优参数 **太多** → 很多部署 **没调就上线**。比较内核性能时须问：**每个内核都调过吗？**

---

### HFT 视角

| 要点 | 说明 |
|------|------|
| **选 Linux** | HFT 圈事实标准 — 社区、工具链、PREEMPT_RT、eBPF 生态 |
| **调优是必须** | 默认配置远非最优 — isolcpus、NOHZ_FULL、大页、IRQ 亲和都要手动设 |
| **不要轻信微基准** | 「内核 X syscall 快 2x」对 HFT 无意义 — 要看 **端到端 tick-to-trade** |
| **版本选择** | LTS + 低延迟补丁（PREEMPT_RT）或云厂商优化内核（AWS Nitro 等） |

 [Ch12 基准测试](../../chapter-12-benchmarking/) · [Ch15 BPF](../../chapter-15-bpf/) · [HFT ch05 内核调优](../../../14-hft-engineering/chapter-05-操作系统内核极致调优/README.md)


---

← [3.5 其他系统模型](./section-3.5-其他系统模型.md) · [本章导读](../README.md)
