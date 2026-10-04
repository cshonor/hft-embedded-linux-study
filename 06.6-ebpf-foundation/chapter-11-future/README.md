# Learning eBPF · 第 11 章：eBPF 的未来演进

> **原书：** Chapter 11: The Future Evolution of eBPF  
> **HFT：** ⚪ · **底本：** LEARNING-EBPF-BILINGUAL.pdf（GPT 双语逐段对照；PDF 存仓库外 `~/Desktop/hft-local-books/`，不入库）

> 底本：LEARNING-EBPF-BILINGUAL.pdf（全书末章）。eBPF 远未定型：它有了跨厂商的中立基金会、正被移植进 Windows、Linux 侧每版内核都在扩展能力。核心论点：**eBPF 是平台，不是功能**——就像容器底层是 namespaces/cgroups 而多数用户无感，未来多数人将通过工具间接使用 eBPF。

## 本章目标

1. 了解 eBPF 基金会的定位与 eBPF 标准化
2. 理解 eBPF for Windows 的架构：许可证约束如何决定组件选型
3. 掌握 Linux 侧三个在研方向：签名程序、长生命周期内核指针、内存分配
4. 建立"新特性 ≠ 生产可用"的内核版本现实感

## 小节索引

| 原书小节 | 笔记 |
|---|---|
| §11.1–11.2 | [11.1 基金会与Windows版](./notes/11.1_基金会与Windows版.md) |
| §11.3–11.4 | [11.2 演进方向与平台观](./notes/11.2_演进方向与平台观.md) |
| §11.5–11.7 | [11.3 坑点HFT关联与自测](./notes/11.3_坑点HFT关联与自测.md) |

## 交叉引用

- 第 1 章 `../chapter-01-what-is-ebpf/`：内核版本与发行版滞后
- 第 5 章 `../chapter-05-core-btf-libbpf/`：重定位机制——签名难题的技术根源
- 第 6 章 `../chapter-06-verifier/`：Linux 验证器行为（对比 PREVAIL）
- 第 9 章 `../chapter-09-security/`：eBPF 缓解内核漏洞、供应链安全语境
- 第 10 章 `../chapter-10-programming/`：按内核版本选特性时的工具链现实

---

**全书完**。11 份章节笔记：01 概念 → 02-04 基础机制（Hello World/程序结构/bpf 系统调用）→ 05-07 核心技术（CO-RE/验证器/程序类型）→ 08-09 应用（网络/安全）→ 10-11 工程与展望。
