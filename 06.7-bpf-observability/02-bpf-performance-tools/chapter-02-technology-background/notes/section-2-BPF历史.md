# 2.2 BPF（经典 BPF 的历史）

> 底本：《BPF之巅》第 2 章技术背景，2.2 节（印刷 p18–19）

## 经典 BPF 是什么

BPF（Berkeley Packet Filter，伯克利数据包过滤器）最早是为 **tcpdump(1) 抓包过滤**设计的：在内核中运行一个小过滤程序，决定哪些数据包复制给抓包程序，避免把所有包都搬到用户态再过滤。

## 时间线

| 时间 | 事件 |
|---|---|
| 1992 | McCanne & Jacobson 发表 BPF 论文（USENIX Winter） |
| 1997 | BPF 进入 Linux 内核 2.1.75，用于 tcpdump/libpcap |
| 2011 | eBPF JIT 出现（内核 2.6.35 为经典 BPF 引入 JIT 之后的演进） |
| 2012 | seccomp 使用经典 BPF |
| 2013–2014 | Alexei Starovoitov 的 eBPF 补丁系列 |
| 3.18 | bpf(2) 系统调用引入，eBPF 正式通用化 |

## 经典 BPF 的限制（也是 eBPF 出现的原因）

- 只有 **2 个寄存器**（A 和 X）、**16 个 32 位槽位**的暂存空间
- 程序只能做包过滤这一件事，无法调用内核函数、无法与用户态共享状态
- 没有映射（map）机制

## HFT 关联

- 经典 BPF 至今活在两处：tcpdump 的包过滤表达式、seccomp 沙箱规则。HFT 系统若用 seccomp 加固交易进程，底层跑的就是经典 BPF（或 eBPF，取决于内核与配置）。
- 理解这段历史的价值在于明白：eBPF 的本质突破不是"过滤包"，而是**把"内核中安全运行小程序"泛化成通用机制**。

## 陷阱

- tcpdump 的 filter 语法（如 `tcp port 8000`）编译出的就是经典 BPF 字节码，与 eBPF 不是一回事，但两者常被混称为"BPF"。

## 自测

<details>
<summary>1. 经典 BPF 有几个寄存器？最初用途是什么？</summary>

2 个（A、X）；最初用途是 tcpdump 的内核态包过滤。
</details>

<details>
<summary>2. 哪个内核版本引入了 bpf(2) 系统调用？</summary>

Linux 3.18。
</details>
