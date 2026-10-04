# Learning eBPF · 第 9 章：eBPF 与安全

> **原书：** Chapter 9: eBPF for Security  
> **HFT：** 🟡 · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 可观测工具只报告事件，安全工具要**区分正常与恶意并采取行动**。本章主线是一条演进链：seccomp → syscall 追踪（Falco）→ BPF LSM → Tetragon 内核函数挂载 + 同步阻断，每一步都在解决上一步的漏洞。

## 本章目标

1. 理解安全可观测性 = 策略（正常/异常判定）+ 上下文（事件发生时的完整信息）
2. 掌握 seccomp-bpf 的工作方式与局限，了解 eBPF 自动生成 seccomp profile 的方法
3. 理解 syscall 入口做安全的致命缺陷：**TOCTOU 竞态窗口**
4. 掌握 BPF LSM（参数已进内核内存后的权威检查点）与 Tetragon 的内核函数挂载思路
5. 理解检测型（异步通知）vs 防护型（`bpf_send_signal` 同步 SIGKILL）安全的区别

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §9.1–9.3 | [9.1 安全模型与syscall方案](./notes/9.1_安全模型与syscall方案.md) |
| §9.4–9.5 | [9.2 Tetragon与防护型安全](./notes/9.2_Tetragon与防护型安全.md) |
| §9.6–9.8 | [9.3 坑点HFT关联与自测](./notes/9.3_坑点HFT关联与自测.md) |

## 交叉引用

- 第 7 章 `../chapter-07-program-attachment-types/`：LSM 程序类型返回码、syscall kprobe、raw tracepoint
- 第 8 章 `../chapter-08-networking/`：XDP 防火墙/DDoS、NetworkPolicy——网络侧防护模式
- 第 6 章 `../chapter-06-verifier/`：为什么 eBPF 程序不能随意解引用用户态指针
- 第 10 章 `../chapter-10-programming/`：gobpf 等 Go 库（本章 OCI hook 示例的用户态实现）
