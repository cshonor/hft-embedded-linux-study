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

| 节 | 笔记 |
|----|------|
| 1. eBPF 基金会（2021） | [notes/section-1-eBPF基金会（2021）.md](./notes/section-1-eBPF基金会（2021）.md) |
| 2. eBPF for Windows | [notes/section-2-eBPFforWindows.md](./notes/section-2-eBPFforWindows.md) |
| 3. Linux eBPF 的演进方向 | [notes/section-3-LinuxeBPF的演进方向.md](./notes/section-3-LinuxeBPF的演进方向.md) |
| 4. "eBPF 是平台，不是功能" | [notes/section-4-eBPF是平台，不是功能.md](./notes/section-4-eBPF是平台，不是功能.md) |
| 5. 坑点清单 | [notes/section-5-坑点清单.md](./notes/section-5-坑点清单.md) |
| 6. HFT 关联 | [notes/section-6-HFT关联.md](./notes/section-6-HFT关联.md) |
| 7. 自测题 | [notes/section-7-自测题.md](./notes/section-7-自测题.md) |

## 交叉引用

- 第 1 章 `../chapter-01-what-is-ebpf/`：内核版本与发行版滞后
- 第 5 章 `../chapter-05-core-btf-libbpf/`：重定位机制——签名难题的技术根源
- 第 6 章 `../chapter-06-verifier/`：Linux 验证器行为（对比 PREVAIL）
- 第 9 章 `../chapter-09-security/`：eBPF 缓解内核漏洞、供应链安全语境
- 第 10 章 `../chapter-10-programming/`：按内核版本选特性时的工具链现实

---

**全书完**。11 份章节笔记：01 概念 → 02-04 基础机制（Hello World/程序结构/bpf 系统调用）→ 05-07 核心技术（CO-RE/验证器/程序类型）→ 08-09 应用（网络/安全）→ 10-11 工程与展望。
