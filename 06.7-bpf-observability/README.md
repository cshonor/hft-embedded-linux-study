# 06.7 · BPF Performance Tools — Brendan Gregg

**文件夹 06.7** · O'Reilly《BPF Performance Tools》（2019）· [返回总清单](../READING-LIST.md)

> **定位：** BCC/bpftrace 工具谱系——按 CPU/内存/IO/网络资源域的观测工具百科与性能分析方法
> **前置（双前置，缺一不可）：**
> [06.6-Learning-eBPF](../06.6-ebpf-foundation/)（会写探针）+ [06.6.5-Systems-Performance](../06.6.5-systems-performance/)（知道观测什么）——本书是前两者的交汇应用，**必须压轴**
> **建议时机：** 已有 Linux 内核/网络/DPDK 或 HFT 压测靶子后再开 — 用 eBPF 验证真实系统
> **后续：** [14-HFT](../14-hft-engineering/) / [18-Rust](../18-rust-quant/)

> 📌 **历史变更（2026-10）：** 本模块原为双书目录，`01-learning-ebpf` 已提为独立模块（现 `06.6-ebpf-foundation`）；本书章节已提升到本目录根级（无 02 层）
> [06.6-ebpf-foundation](../06.6-ebpf-foundation/)（阅读顺序 06.6 → 06.6.5 → 06.7）。

---

## 目录结构

```
06.7-bpf-observability/       ← 单书模块：BPF Performance Tools（章节已提升至根级）
├── OUTLINE.md                全书目录 + HFT 读/跳标注
├── BPF-PERFORMANCE-TOOLS-EVAL.md
├── BOOK-TOC.md / SUPPLEMENT-web-resources.md
├── chapter-01~18/            章导读 + 分节笔记 + code/（.bt 程序与脚本）
├── appendix-A~E              bpftrace/BCC/指令附录
├── note-XDP与tc-BPF.md       HFT 延伸
├── EBPF-BOOKS-COMPARISON.md  ← eBPF 三书对比与协同读法
├── ref-*.md                  ← 模块级参考（bpftrace 脚本/排查决策树/评审清单）
└── README.md
```

目录约定（与 [02-CSAPP](../02-computer-systems/) · [06.6.5-SysPerf](../06.6.5-systems-performance/) 一致）：`chapter-XX-english-slug/README.md`（章导读）+ `notes/`（按节拆分的笔记）+ `code/`（示例程序）。

---

## 为什么先读 06.6 与 06.6.5

1. **先懂原理，再用工具。** 本目录的 bpftrace/BCC 工具，底层就是 Learning eBPF 第 7 章的程序/附加类型、第 6 章的验证器、第 2 章的 map。先读原理书，用工具报 "invalid mem access" 时能直接读懂验证器日志，而不是当黑盒。

2. **先知道观测什么，再拿工具。** BPT Ch3 的 60s 清单、USE 方法就是 SysPerf 的框架——没有方法论，工具只是命令列表。

3. **新旧顺序：先学现代标准。** Learning eBPF（2023）代表 libbpf + CO-RE 现代栈；BPT（2019）停在 BCC 运行时编译范式。先掌握现代标准，再读 BPT 时能分清哪些是历史写法、哪些工具思路至今有效。

4. **能力递进闭环：先会造，再会用。** Learning eBPF 教你**写**定制观测程序（HFT 需要非标准挂点，如行情网卡入口的 XDP、解析库的 uprobe）；BPT 教你**用**成熟工具快速覆盖标准场景（调度/缺页/TCP 重传）。

> 详细对比（含第三本 Linux Observability with BPF）见 [EBPF-BOOKS-COMPARISON.md](./EBPF-BOOKS-COMPARISON.md)。

---

## 内部导航

各章导读见 [02-bpf-performance-tools/OUTLINE.md](./OUTLINE.md)（含 🔴🟡⚪ HFT 读/跳标注）。

**HFT 精读捷径：**

```
Ch 1–2 → Ch 4–5 → Ch 6 → Ch 10 (+ note-XDP) → 附录 A/B
```

## 模块级参考文档

| 文档 | 用途 |
|------|------|
| [bpftrace 样例脚本集](./ref-bpftrace-scripts.md) | 8 个场景脚本：调度/IO/TCP/syscall/锁/缺页/slab/软中断 |
| [故障排查决策树](./ref-troubleshooting-decision-tree.md) | 7 大症状入口 → 19 看现象 → 20 钻根因 |
| [Rubric 评审校验清单](./ref-rubric-checklist.md) | 一次排障/优化做完后逐项打勾（6 大类 30+ 检查项） |

## 交叉阅读

- **双前置** → [06.6-ebpf-foundation](../06.6-ebpf-foundation/) · [06.6.5-systems-performance](../06.6.5-systems-performance/)
- 后续内核/内存/网络 → [05-linux-kernel](../05-linux-kernel/) · [06-linux-mm](../06-linux-mm/) · [12-kernel-networking](../12-kernel-networking/)（读时可回头用 eBPF 验证）
- DPDK 对照 → [13-dpdk](../13-dpdk/)（XDP early drop vs 用户态旁路）
- Rust eBPF → [18-rust-quant](../18-rust-quant/)（Aya/bpf2go）
- 跨模块 → [README.md](../README.md)

---

## 本机实跑验证记录（2026-10 · Ubuntu 24.04 / kernel 7.0.0-38-generic / bpftrace root 批量实测）

全部 **33 个 `.bt` 逐一 root 实跑：32 OK + 1 设计性例外**（`rename-exchange-kill.bt` 需 `--unsafe`，单独验证可 attach）。.bash 全量 `bash -n` ✅、.py 全量 `py_compile` ✅。

书中例子写于 kernel 4.x/5.x 时代，在**内核 7.0 上需要以下适配**（已全部修复并在文件头注释标注）：

| 失效类别 | 涉及脚本 | 修法 |
|---|---|---|
| 内核头 `static_assert` 解析失败 | faults-by-file / oomkill / read-by-fs / vfs-read-by-file / slab-name / tcp-state-trace | 不 `#include`，类型走 BTF；TCP 宏改字面量 |
| kprobe 符号消失/改名 | slab-name（`kmem_cache_alloc` 内联）/ blkthrot（`blk_throtl_bio` 重构）/ blk-plug（`blk_flush_plug_list` 内联）/ numa-migrate（改 folio 版） | 换稳定 tracepoint 或现存符号 |
| `finish_task_switch` 进 kprobe 黑名单 | offcpu / pidns-switch | 改 `tracepoint:sched:sched_switch` 语义等价模型 |
| bpftrace 语法/API | runqlat（if 必须带 `{}`）/ mem-expand（filter 合并）/ pagefault-ustack（`page-faults` 复数）/ tcp-state-trace（无 `lport()`）/ pidns-switch（nodename 已是 string） | 逐一修复 |

**经验**：非 root 的语法检查覆盖不到这些——bpftrace 的 root 检查先于解析报错，
只有 root 实跑能暴露。runqlat 实测直方图（调度延迟峰值 8–16µs）与笔记"热核 < 数十 µs"一致。
