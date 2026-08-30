# eBPF 与 BPF 的称呼

> 本节讲什么：一个常见的困惑——资料里一会儿写 BPF 一会儿写 eBPF，是不是两个东西？结论先行：**现代语境下是同一个东西**，但用词场合有惯例。

## 1. 两个词的来历

- **BPF**：1993 年的原名（BSD Packet Filter），32 位"伪机器"时代的叫法。今天为了区分，常称 **classic BPF（cBPF）**
- **eBPF**：2014 年 64 位重写后的叫法，e = extended。内核之外的社区（ebpf.io、eBPF 基金会、O'Reilly 书名）用它强调"不只是包过滤了"

## 2. 现状：谁用哪个词

| 场合 | 用词 | 例子 |
|---|---|---|
| 内核源码、内核邮件列表 | 统一 **BPF** | `bpf()` 系统调用、`bpf_get_current_pid_tgid()` 等 helper、`BPF_PROG_TYPE_*` 程序类型——**内核里根本没有 "eBPF" 这个词** |
| 工具链（libbpf/bpftool/clang） | 统一 **BPF** | `clang -target bpf`、`bpftool prog load` |
| 内核外社区、营销、书名 | 流行 **eBPF** | ebpf.io、eBPF Summit、本书《Learning eBPF》 |
| bpftrace 等高层工具 | 混用 | 无所谓 |

## 3. 为什么内核社区坚持不加 "e"

内核维护者的立场：cBPF 在内核里已经被**完全翻译成 eBPF 再执行**（cBPF 程序加载时先被转换），也就是说内核里只有一种 BPF——即 2014 年后的那套。旧名没有独立存在的意义，所以"extended"前缀多余。

**实用结论**：
- 读内核文档/写 C 代码：心里按 BPF 理解，别找 `ebpf_xxx` 符号（不存在）
- 读博客/书：见到 eBPF 直接当 BPF 即可
- 写搜索关键词：两个都试（如 `bpf ring buffer` / `ebpf ring buffer`）

---

**衔接**：术语清了，下一节回到地基问题——为什么"在内核里执行"这件事价值巨大？先看用户态/内核态的边界长什么样。
