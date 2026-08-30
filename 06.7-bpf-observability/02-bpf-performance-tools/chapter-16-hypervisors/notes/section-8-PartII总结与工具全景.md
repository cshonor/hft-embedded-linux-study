# 8. Part II 总结与工具全景

> 底本：《BPF之巅》第二部分（第 11–16 章）收官回顾，依托第 16 章 16.5 节（印刷 p737）

第 16 章是第二部分（BPF 工具，第 11–16 章）的最后一章。第二部分按**计算环境栈**逐层展开，每章都是"背景知识 → 传统工具 → BPF 工具 → 单行程序 → 小结"的结构：

| 章 | 环境 | 代表工具/主题 |
|---|---|---|
| 11 | 安全 | execsnoop、opensnoop、tcpaccept、tcpconnect |
| 12 | 编程语言 | profile/offcputime（栈翻译）、funcalls、 javastat 类 |
| 13 | 应用程序 | runqlat 配套 CPU/OffCPU 剖析、mysqld_qslower、锁与睡眠排障 |
| 14 | 内核 | 唤醒链 offwaketime、内核锁 mlock/mheld、kmem/slabratetop、workq |
| 15 | 容器 | pidnss 容器标识、runqlat --pidnss、blkthrot、overlayfs 延迟 |
| 16 | 虚拟机管理器 | xenhyper、cpustolen、kvmexits、超级调用/退出/盗用时间 |

## Part II 的方法论沉淀

1. **先环境后工具**：先弄清目标所处层级（应用/语言运行时/内核/容器/VM），再选择该层的专用工具；跨层问题（容器内应用慢）需要多层同时取证
2. **传统工具先行**：每章的传统工具（top/perf/slabtop/perf kvm …）给出快速概览，BPF 工具负责分布、事件级细节与因果链
3. **三个通用构件**：计数（funccount/count）、延迟直方图（hist + 事件对配对）、调用栈（stackcount/ustack/kstack）——从应用到内核再到 hypervisor 层层复用
4. **可见性与性能的权衡**：越是优化的环境（内联、硬件化、Nitro），可跟踪目标越少——第 16 章 PV→Nitro 的收缩曲线是全书的缩影

## 自此向后

- **第三部分（17–18 章 + 附录）**：其他 BPF 工具（Vector/PCP、Grafana、eBPF exporter、kubectl-trace 等图形化/云环境前端）、技巧与提示、附录（bpftrace 单行、备忘单、BCC 开发、C 语言 BPF、BPF 指令集）
- 第 17 章起视角从"单机命令行"转向"成千上万实例的云计算 GUI"——但底层还是第 4–16 章的同一批 BCC/bpftrace 工具

## HFT 关联

- 第二部分给出的是一套**分层取证流水线**：HFT 生产问题（尾延迟、丢包、steal）按 13（应用）→ 11/12（安全/语言）→ 14（内核）→ 15（容器）→ 16（VM）的顺序逐层定位
- 建议把各章"小结速查表"合并为团队的一页排障决策树（可与本仓库 ref-troubleshooting-decision-tree.md 对照互补）
