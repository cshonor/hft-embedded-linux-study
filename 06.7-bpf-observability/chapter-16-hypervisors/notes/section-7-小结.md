# 7. 小结（16.5）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.5 节（印刷 p737）

本章总结了**硬件虚拟机管理器**，并说明了 BPF 跟踪如何从**访客系统**和**宿主机**中揭露详细信息，包括：

- **超级调用**（16.3.1–16.3.2：funccount/argdist/stackcount/funclatency 插桩 xen 跟踪点与 multicall 机制、xenhyper 按名称计数）
- **被盗用的 CPU 时间**（16.3.4：cpustolen 用 kretprobe xen_stealclock/kvm_stealclock 差值直方图）
- **访客系统的退出**（16.4.1：kvmexits 用 kvm:kvm_exit / kvm_entry 跟踪点对按原因出直方图）

## 本章工具速查

| 工具 | 类型 | 位置 | 一句话用途 |
|---|---|---|---|
| `funccount 't:xen:*'` | 单行 | 访客（PV） | 列出 Xen 跟踪点及触发次数 |
| `argdist ... args->mcidx` | 单行 | 访客（PV） | 每次 multicall flush 的超级调用数 |
| `stackcount t:xen:xen_mc_issue` | 单行 | 访客（PV） | 超级调用来源调用栈（do_fork→xen_set_pte_at） |
| `funclatency xen_mc_flush` | 单行 | 访客（PV） | 真正超级调用延迟（512–1023ns 主体） |
| xenhyper(8) | bpftrace | 访客（PV） | 按名称统计超级调用（stack_switch/mmuext_op…） |
| kprobe xen_evtchn_do_upcall | 单行 | 访客（Xen） | 回调中断了谁、回调延迟（1–32us） |
| cpustolen(8) | bpftrace | 访客（Xen/KVM） | 被盗用 CPU 时间分布（短期 vs 长期） |
| kvmexits(8) | bpftrace | 宿主机（KVM） | 按退出原因的 VM exit 时长直方图 |
| `t:kvm:kvm_exit /…30…/` guest_rip | 单行 | 宿主机（KVM） | I/O 退出时的访客指令指针采样 |
| perf kvm stat live | 传统 | 宿主机（KVM） | 每退出原因的 min/avg/max（BPF 前的方案） |

## 贯穿本章的两个模式

1. **手写映射表**：xenhyper 的 @name[0..33]（xen-hypercalls.h）与 kvmexits 的 @exit_reason[0..58]（vmx.h）——内核没有为这类编号提供符号，需从内核头文件抄录并随版本维护
2. **可观测性随硬件化而收缩**：PV（超级调用+回调+盗用）→ PVHVM（回调+盗用）→ SR-IOV（回调+盗用）→ Nitro（仅盗用）。管理器专用工具的分析空间越来越小，越来越依赖前面章节的通用资源工具

## HFT 关联

- 云上 HFT 部署的观测边界由实例类型决定；对延迟最敏感的组件应使用裸金属/专用主机，从根源上消除 steal 与退出
- 自建虚拟化平台排障清单：访客内（xenhyper / cpustolen / 通用工具）+ 宿主机（kvmexits / perf kvm stat）两侧同时取证

<details>
<summary>自测题</summary>

1. 本章 BPF 工具分别覆盖了访客系统与宿主机的哪三类信息？
2. xenhyper 与 kvmexits 在实现上的共同"模式"是什么？各自的映射表来自哪个头文件？

<details><summary>参考答案</summary>

1. 超级调用（xenhyper 计数/funclatency 延迟/stackcount 来源）、被盗用 CPU 时间（cpustolen 差值直方图）、VM 退出（kvmexits 按原因直方图，宿主机侧）。
2. **手写编号→名称映射表**：跟踪点/kprobe 只给数字编号（hypercall 号、exit reason 号），内核不提供符号——从内核头文件抄录成 @name[N] 哈希，随内核版本手动维护。xenhyper 用 xen-hypercalls.h（0..33），kvmexits 用 vmx.h 的 exit reason（0..58）。这也是 11 章 eperm 读 sys_call_table 的对照：那边让内核自己翻译，这边只能人肉维护。
</details>
</details>
