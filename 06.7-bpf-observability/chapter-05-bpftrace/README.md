# Ch 5 bpftrace · bpftrace

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**ad hoc 排障与短脚本的入口** — 若 [Ch 4 BCC](../chapter-04-bcc/) 是写复杂工具、守护进程的**重型武器**，bpftrace 则适合**临时验证假设、单行命令（one-liners）、几十行短脚本**。语法类似 **awk + C**，大幅降低 eBPF 门槛。
> **HFT：** 排障假设的快速验证器（一行验证一个猜想）；交易机最小依赖部署的观测利器。
> **上一章：** [chapter-04-BCC](../chapter-04-bcc/) · **下一章：** [chapter-06-CPU](../chapter-06-cpus/)

---

## 小节笔记（按原书 5.1–5.18 真实目录）

| 节 | 原书小节 | 笔记 |
|----|----------|------|
| 5.1 | bpftrace 的组件 | [section-1-bpftrace的组件.md](./notes/section-1-bpftrace的组件.md) |
| 5.2 | bpftrace 的特性 | [section-2-bpftrace的特性.md](./notes/section-2-bpftrace的特性.md) |
| 5.3 | bpftrace 的安装 | [section-3-bpftrace的安装.md](./notes/section-3-bpftrace的安装.md) |
| 5.4 | bpftrace 工具 | [section-4-bpftrace工具.md](./notes/section-4-bpftrace工具.md) |
| 5.5 | bpftrace 单行程序 | [section-5-bpftrace单行程序.md](./notes/section-5-bpftrace单行程序.md) |
| 5.6 | bpftrace 的文档 | [section-6-bpftrace的文档.md](./notes/section-6-bpftrace的文档.md) |
| 5.7 | bpftrace 编程 | [section-7-bpftrace编程.md](./notes/section-7-bpftrace编程.md) |
| 5.8 | bpftrace 的帮助信息 | [section-8-bpftrace的帮助信息.md](./notes/section-8-bpftrace的帮助信息.md) |
| 5.9 | bpftrace 的探针类型 | [section-9-bpftrace的探针类型.md](./notes/section-9-bpftrace的探针类型.md) |
| 5.10 | bpftrace 的控制流 | [section-10-bpftrace的控制流.md](./notes/section-10-bpftrace的控制流.md) |
| 5.11 | bpftrace 的运算符 | [section-11-bpftrace的运算符.md](./notes/section-11-bpftrace的运算符.md) |
| 5.12 | bpftrace 的变量 | [section-12-bpftrace的变量.md](./notes/section-12-bpftrace的变量.md) |
| 5.13 | bpftrace 的函数 | [section-13-bpftrace的函数.md](./notes/section-13-bpftrace的函数.md) |
| 5.14 | bpftrace 映射表的操作函数 | [section-14-bpftrace映射表的操作函数.md](./notes/section-14-bpftrace映射表的操作函数.md) |
| 5.15 | bpftrace 的下一步工作 | [section-15-bpftrace的下一步工作.md](./notes/section-15-bpftrace的下一步工作.md) |
| 5.16 | bpftrace 的内部运作 | [section-16-bpftrace的内部运作.md](./notes/section-16-bpftrace的内部运作.md) |
| 5.17 | bpftrace 的调试 | [section-17-bpftrace的调试.md](./notes/section-17-bpftrace的调试.md) |
| 5.18 | 小结 | [section-18-小结.md](./notes/section-18-小结.md) |

---

## 双探针计时模板（本章精华）

```awk
kprobe:fn  { @start[tid] = nsecs; }
kretprobe:fn /@start[tid]/
{
    @us = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}
```

三个要点：`tid` 为键防多线程覆盖；`/@start[tid]/` 过滤防"入口未记录"假离群点；映射表名带单位。

## 本章 Checklist（HFT 视角）

- [ ] **通配符先 `-l` 预览**——BPFTRACE_MAXPROBES 默认 512，超限被拒。
- [ ] **计时必加 `/@start[tid]/`**——离群点九成来自入口未记录。
- [ ] **sum() 前滤负值**——read 等负返回是 -errno；先存纳秒、print 时用 div 除，避免整数截断。
- [ ] **自动化脚本三要素**——`interval+exit()` 限时长、`/过滤/` 写内核态、映射表名带单位。
- [ ] **str() 默认 64B**——长路径调 BPFTRACE_STRLEN（老版 200B 硬上限）。
- [ ] **system() 需 --unsafe 且高频探针下是性能炸弹**。
- [ ] **pid=tgid、tid=内核 pid**——多线程统计口径别搞反。
- [ ] **探针优先级**——生产用 tracepoint（稳定 ABI）> kprobe（随内核版本变）。

---

## 相关章节

- 上一章：[chapter-04-BCC](../chapter-04-bcc/)
- 下一章：[chapter-06-CPU](../chapter-06-cpus/)
- 技术地基：[chapter-02-技术背景](../chapter-02-technology-background/)
- 单行宝典：[appendix-A-bpftrace单行命令](../appendix-A-bpftrace单行命令.md)
- 备忘单：[appendix-B-bpftrace备忘单](../appendix-B-bpftrace备忘单.md)
