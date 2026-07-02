# 1. BCC 是什么

| 维度 | 说明 |
|------|------|
| **定位** | 构建 BPF 软件的 **开源编译器框架 + 工具集** |
| **前端语言** | Python、C++、Lua |
| **编译链** | **Clang/LLVM** 将 BPF C 编译为字节码 → `bpf()` 注入内核 |
| **规模** | **70+** 单用途工具 + 若干多用途「瑞士军刀」 |

**与全书关系：** [Ch 2](../../chapter-02-technology-background/) 讲 VM/Map/探针原理；[Ch 3](../../chapter-03-performance-analysis/) 给 BCC 工具 **检查清单**；本章讲 **BCC 生态本身** 与四大多用途工具；[Ch 5](../../chapter-05-bpftrace/) 讲更轻量的 **bpftrace** 脚本语言。

---
