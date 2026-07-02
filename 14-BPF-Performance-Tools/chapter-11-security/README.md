# Ch 11 安全 · Security

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**安全分析与性能工程的交叉** — eBPF 源于 **包过滤 / 防火墙 / IDS**；本章展示 BPF 用于 **入侵检测、行为白名单、权限最小化、实时取证**。许多工具与 [Ch 6](../chapter-06-cpus/)/[Ch 8](../chapter-08-file-systems/)/[Ch 10](../chapter-10-networking/) **同名复用**（`execsnoop`、`opensnoop`、`tcpconnect`），视角从「性能」转为「谁干了什么」。  
> **HFT：** 生产机 **⚪ 默认跳过** 精读；与 **合规/共置隔离** 相关时查阅 **`capable`/`setuids`/`tcpconnect`** 做 **最小权限白名单**；**零日应急响应** 用 bpftrace 快速写 probe。勿与低延迟热路径 **长期** 同机全开。  
> **上一章：** [chapter-10-网络.md](../chapter-10-networking/) · **下一章：** [chapter-12-语言.md](../chapter-12-languages/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 安全与 BPF 的共性 | [notes/section-1-安全与BPF的共性.md](./notes/section-1-安全与BPF的共性.md) |
| 2 BPF 在安全上的优势 | [notes/section-2-BPF在安全上的优势.md](./notes/section-2-BPF在安全上的优势.md) |
| 3 进程与模块执行 | [notes/section-3-进程与模块执行.md](./notes/section-3-进程与模块执行.md) |
| 4 终端与 Shell 监控（实时取证） | [notes/section-4-终端与Shell监控实时取证.md](./notes/section-4-终端与Shell监控实时取证.md) |
| 5 权限与能力 (Capabilities) | [notes/section-5-权限与能力.md](./notes/section-5-权限与能力.md) |
| 6 网络与文件异常 | [notes/section-6-网络与文件异常.md](./notes/section-6-网络与文件异常.md) |
| 7 BPF 单行命令 (One-Liners) | [notes/section-7-BPF单行命令.md](./notes/section-7-BPF单行命令.md) |
| 8 工具选型速查（安全视角） | [notes/section-8-工具选型速查安全视角.md](./notes/section-8-工具选型速查安全视角.md) |
| 9 与性能章节的工具对照 | [notes/section-9-与性能章节的工具对照.md](./notes/section-9-与性能章节的工具对照.md) |

---

## 大白话

> 安全分析与性能工程的交叉

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **⚪ 默认跳过**— 除非合规、共置审计、post-incident 取证。
- [ ] **`capable` + 最小 cap**— 新二进制上线前的 **权限摸底**，比事后 `setuids` 告警更省事。
- [ ] **`tcpconnect`/`opensnoop`**— 策略机 **不应外连/读敏感路径**；与 Ch 10/8 工具相同，可纳入 **轻量合规巡检**（低频 cron，非 tick 路径）。
- [ ] **BPF 比 auditd 轻**— 若必须用 syscall 审计，优先评估 BPF 方案对延迟的影响。
- [ ] **`kernel.unprivileged_bpf_disabled=1`**— 生产推荐；观测脚本由 **受控 root/automation** 运行。
- [ ] **勿在最低延迟核长期挂安全 probe**— 与性能观测同一纪律：短窗口、限 scope。

---

## 相关章节

- 上一章：[chapter-10-网络.md](../chapter-10-networking/)
- 下一章：[chapter-12-语言.md](../chapter-12-languages/)
- execsnoop：[chapter-06-CPU.md](../chapter-06-cpus/)
- opensnoop：[chapter-08-文件系统.md](../chapter-08-file-systems/)
- tcpconnect：[chapter-10-网络.md](../chapter-10-networking/)
- bpftrace：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
