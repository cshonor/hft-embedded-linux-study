# Ch 11 安全 · Security

> **BPF Performance Tools** · Brendan Gregg · 印刷 p516–544

> 本章定位：**安全分析与性能工程的交叉** — eBPF 源于包过滤/防火墙/IDS；本章展示 BPF 用于**实时取证、权限最小化、白名单、入侵检测、零日应急**。许多工具与 [Ch 6](../chapter-06-cpus/)/[Ch 8](../chapter-08-file-systems/)/[Ch 10](../chapter-10-networking/) 同名复用（`execsnoop`、`opensnoop`、`tcpconnect`），视角从「性能」转为「谁干了什么」。
> **HFT：** 生产交易机默认低频使用；与**合规/共置隔离**相关时用 **`capable`/`setuids`/`tcpconnect`** 做最小权限白名单；**零日应急**用 bpftrace 快速写 probe。勿与低延迟热路径长期同机全开。
> **上一章：** [chapter-10-networking/](../chapter-10-networking/) · **下一章：** [chapter-12-languages/](../chapter-12-languages/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 内容 |
|----|------|------|
| 11.1.1 分析能力 | [section-1-背景知识](./notes/section-1-背景知识.md) | 图 11-1 监控目标 · 零日检测（Docker renameat2）· LLM 对比与 1/6 开销 · 事件洪水防御 · seccomp/Cilium/bpfilter/Landlock/KRSI · bpf_send_signal |
| 11.1.2–11.1.4 | [section-2-BPF安全配置与分析策略](./notes/section-2-BPF安全配置与分析策略.md) | 无特权 BPF · 5 个 sysctl（jit/harden/kallsyms/limit）· 三级分析策略 |
| 11.2.1–11.2.4 | [section-3-BPF工具-进程与命令执行](./notes/section-3-BPF工具-进程与命令执行.md) | execsnoop · elfsnoop（mount+inode）· modsnoop · bashreadline |
| 11.2.5–11.2.6 | [section-4-BPF工具-会话取证](./notes/section-4-BPF工具-会话取证.md) | shellsnoop（-r 重放）· ttysnoop |
| 11.2.7–11.2.8 | [section-5-BPF工具-文件与权限拒绝](./notes/section-5-BPF工具-文件与权限拒绝.md) | opensnoop · eperm（sys_call_table 反查） |
| 11.2.11–11.2.12 | [section-6-BPF工具-能力与提权](./notes/section-6-BPF工具-能力与提权.md) | capable（白名单+栈）· setuids（入口/出口配对） |
| 11.2.9–11.2.10 | [section-7-BPF工具-网络活动](./notes/section-7-BPF工具-网络活动.md) | tcpconnect/tcpaccept · tcpreset（端口扫描检测） |
| 11.3 | [section-8-BPF单行程序](./notes/section-8-BPF单行程序.md) | funccount security_* · PAM 跟踪 · 模块加载 |
| 11.4 | [section-9-小结](./notes/section-9-小结.md) | 三大能力 · 工具复用关系 · HFT 三板斧 |

---

## 本章 Checklist

- [ ] **`capable` + 最小 cap**— 新二进制上线前权限摸底，生成 cap_drop/cap_add 清单。
- [ ] **`tcpconnect`/`opensnoop`**— 策略机不应外连/读敏感路径；轻量合规巡检（低频 cron，非 tick 路径）。
- [ ] **`kernel.unprivileged_bpf_disabled=1` + `bpf_jit_enable=1`**— 生产基线；观测脚本由受控 root 运行。
- [ ] **事件洪水防御**— 安全类 BPF 必须监控映射溢出（每 CPU 计数器不丢计数）。
- [ ] **BPF 比 auditd 轻（1/6）**— 必须 syscall 审计时优先评估 BPF。
- [ ] **勿在最低延迟核长期挂安全 probe**— 短窗口、限 scope。

---

## 相关章节

- 上一章：[chapter-10-networking/](../chapter-10-networking/)
- 下一章：[chapter-12-languages/](../chapter-12-languages/)
- execsnoop：[chapter-06-cpus/](../chapter-06-cpus/) · opensnoop：[chapter-08-file-systems/](../chapter-08-file-systems/) · tcpconnect：[chapter-10-networking/](../chapter-10-networking/)
- 容器内 BPF：chapter-15-containers/
