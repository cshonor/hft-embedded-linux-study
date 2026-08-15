# Learning eBPF · 第 9 章：eBPF 与安全

> **原书：** Chapter 9: eBPF for Security  
> **HFT：** 🟡 · **底本：** [LEARNING-EBPF-BILINGUAL.pdf](../LEARNING-EBPF-BILINGUAL.pdf)（GPT 双语逐段对照）

> 可观测工具只报告事件，安全工具要**区分正常与恶意并采取行动**。本章主线是一条演进链：seccomp → syscall 追踪（Falco）→ BPF LSM → Tetragon 内核函数挂载 + 同步阻断，每一步都在解决上一步的漏洞。

## 本章目标

1. 理解安全可观测性 = 策略（正常/异常判定）+ 上下文（事件发生时的完整信息）
2. 掌握 seccomp-bpf 的工作方式与局限，了解 eBPF 自动生成 seccomp profile 的方法
3. 理解 syscall 入口做安全的致命缺陷：**TOCTOU 竞态窗口**
4. 掌握 BPF LSM（参数已进内核内存后的权威检查点）与 Tetragon 的内核函数挂载思路
5. 理解检测型（异步通知）vs 防护型（`bpf_send_signal` 同步 SIGKILL）安全的区别

## 小节索引

| 节 | 笔记 |
|----|------|
| 1. 安全可观测性 = 策略 + 上下文 | [notes/section-1-安全可观测性=策略+上下文.md](./notes/section-1-安全可观测性=策略+上下文.md) |
| 2. 基于系统调用的安全（三代方案） | [notes/section-2-基于系统调用的安全（三代方案）.md](./notes/section-2-基于系统调用的安全（三代方案）.md) |
| 3. BPF LSM：权威检查点（5.7+） | [notes/section-3-BPFLSM：权威检查点（5.7+）.md](./notes/section-3-BPFLSM：权威检查点（5.7+）.md) |
| 4. Cilium Tetragon：挂内核内部函数 | [notes/section-4-CiliumTetragon：挂内核内部函数.md](./notes/section-4-CiliumTetragon：挂内核内部函数.md) |
| 5. 防护型安全（preventative） | [notes/section-5-防护型安全（preventative）.md](./notes/section-5-防护型安全（preventative）.md) |
| 6. 坑点清单 | [notes/section-6-坑点清单.md](./notes/section-6-坑点清单.md) |
| 7. HFT 关联 | [notes/section-7-HFT关联.md](./notes/section-7-HFT关联.md) |
| 8. 自测题 | [notes/section-8-自测题.md](./notes/section-8-自测题.md) |

## 交叉引用

- 第 7 章 `../chapter-07-program-attachment-types/`：LSM 程序类型返回码、syscall kprobe、raw tracepoint
- 第 8 章 `../chapter-08-networking/`：XDP 防火墙/DDoS、NetworkPolicy——网络侧防护模式
- 第 6 章 `../chapter-06-verifier/`：为什么 eBPF 程序不能随意解引用用户态指针
- 第 10 章 `../chapter-10-programming/`：gobpf 等 Go 库（本章 OCI hook 示例的用户态实现）
