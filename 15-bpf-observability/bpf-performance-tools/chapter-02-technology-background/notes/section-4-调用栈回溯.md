# 2.4 调用栈回溯（帧指针 / DWARF / LBR / ORC / 符号）

> 底本：《BPF之巅》第 2 章技术背景，2.4 节（印刷 p35–42，含 2.4.1–2.4.6）

性能问题定位到"哪段代码"最依赖调用栈。本节讲清四种栈回溯技术与符号化。

## 2.4.1 基于帧指针的回溯

- 机制：编译器保留 RBP（帧指针），每帧把上一帧 RBP 压栈，形成链表；回溯 = 沿 RBP 链逐帧取返回地址。
- 特点：**开销最小、实现最简单、可靠**（除非栈被破坏）。
- 问题：gcc/clang 默认 `-fomit-frame-pointer`（省一个寄存器、性能略增），发行版内核与大量用户态程序默认无帧指针 → 链断。
- 对策：编译时加 `-fno-omit-frame-pointer`（发行版如 Ubuntu 后来也为内核保留了 FP）。

## 2.4.2 调试信息（DWARF）

- 不改代码，靠 `.debug_*` 段（DWARF）描述"如何展开栈"。
- 代价：调试信息巨大。书中对比：libjvm 带调试信息约 17MB → 完整 debuginfo 约 222MB。BCC/bpftrace 直接用 DWARF 做用户态栈回溯支持有限（内核侧用 DWARF 不可行）。

## 2.4.3 最后分支记录（LBR）

- Intel CPU 硬件记录最近 N 条分支（4–32 深度，随代际增长）。
- perf 可以用它做栈回溯，不依赖任何编译选项；缺点是深度有限（深栈只能取到最顶部若干帧）。

## 2.4.4 ORC（Oops Rewind Capability）

- x86_64 内核专用（2018，Linux 4.6 起逐步启用）：内核编译时生成 `.orc_unwind` 段，比 DWARF 小得多、查找快。
- `dmesg` Oops 的栈回溯、BPF 栈回溯（`bpf_get_stack`）在无帧指针内核上靠 ORC。

## 2.4.5 符号

栈上拿到的是地址，需翻译成"函数名+偏移"。内核符号在 /proc/kallsyms（需 root 看到真实地址）；用户态符号在 ELF symtab（未 strip 才有），运行地址 = 符号 + 加载基址（PIE 时需解析 /proc/<pid>/maps）。

## 2.4.6 扩展阅读

帧指针保留争论（Linus 的著名邮件）、DWARF 标准、LBR 论文/手册、ORC 补丁系列。

## 四种技术对比表

| 技术 | 依赖 | 深度 | 开销 | 适用 |
|---|---|---|---|---|
| 帧指针 | 编译保留 RBP | 无限 | 极低 | 内核+用户态，首选 |
| DWARF | debuginfo 文件 | 无限 | 中（查找） | 用户态离线分析 |
| LBR | Intel CPU | 4–32 帧 | 低 | 深栈不够用的快速采样 |
| ORC | .orc_unwind（内核自带） | 无限 | 低 | 无 FP 的 x86_64 内核 |

## HFT 关联

- HFT 交易路径上每纳秒都贵，历史倾向 `-fomit-frame-pointer` 抠性能 → 出问题时没有栈。常见折衷：**生产版保留帧指针**（RBP 占用约 1 寄存器的代价，通常 <1%，换来全程可观测），或至少 debug 构建保留。
- 延迟尖刺定位三板斧之一就是"尖刺时刻的内核+用户态栈采样"，帧指针是最低开销的可用性保证。
- kallsyms 需 root 才显示真实地址：容器化部署的交易服务若非特权运行，栈会全是 0 —— 需提前规划。

## 陷阱

- 用 perf record 抓用户态栈全是 `[unknown]`：目标二进制没帧指针且无 debuginfo。
- strip 过的 so 库让符号化失效 → 部署时应保留 symtab 或单独存放 debuginfo。
- LBR 深度浅，误把"栈顶若干帧"当完整调用路径。

## 自测

<details>
<summary>1. gcc 默认是否保留帧指针？如何开启？</summary>

默认 omit（-fomit-frame-pointer）。加 -fno-omit-frame-pointer 保留 RBP。
</details>

<details>
<summary>2. ORC 是给谁用的？为什么不用 DWARF？</summary>

x86_64 内核栈回溯。DWARF 体积大、查找慢，不适合内核 Oops 场景；ORC 表小且查找 O(1) 级。
</details>

<details>
<summary>3. 栈回溯拿到的地址如何变成函数名？</summary>

内核：/proc/kallsyms 查符号区间；用户态：ELF symtab + 加载基址（PIE 需解析 /proc/<pid>/maps）。
</details>
