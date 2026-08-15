# 7. BCC vs bpftrace（预告）

| | **BCC** | **bpftrace** |
|---|---------|--------------|
| **形态** | Python + 嵌入 BPF C | **一门脚本语言** |
| **工具数量** | 70+ 预置 + 可自研 | 单行 ad hoc 极强 |
| **本书** | **本章** | [Ch 5](../../chapter-05-bpftrace/) |
| **HFT 分工** | runbook 固定工具、复杂多文件工具 | 验证假设、临时计数/直方图 |

**原则：** 先熟练 **单用途 BCC 清单** → 再学 **四个多用途** → 再用 **bpftrace** 补洞（附录 A）。


### 常见陷阱

1. **以为 BCC 和 bpftrace 是竞争关系** — 两者互补：BCC 适合复杂工具开发和团队标准化，bpftrace 适合快速 one-liner 和探索性分析
2. **忽视 bpftrace 的学习曲线** — bpftrace 的 DSL 语法简洁但需要理解探针类型、变量作用域、Map 操作；投资学习后效率远超 BCC
3. **在新项目中选择 BCC 而非 bpftrace** — 新项目（2024+）推荐 bpftrace 或 libbpf+CO-RE；BCC 的运行时编译模型逐渐被视为遗留方案

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 和 bpftrace 的核心区别是什么？**

   <details>
   <summary>参考答案</summary>

   BCC：Python+C 框架，运行时编译，适合复杂工具开发，部署重（需 Clang+headers），启动慢（1-3s）。bpftrace：专用 DSL，预编译探针描述，适合 one-liner 和快速分析，部署轻（单一二进制），启动快。互补关系，不是替代。

   </details>

2. **在什么场景下应该选择 bpftrace 而非 BCC？**

   <details>
   <summary>参考答案</summary>

   (1) 快速验证假设（one-liner 秒级出结果）；(2) 简单聚合/直方图（count/sum/hist）；(3) 临时排障无需持久化工具；(4) 环境无 Clang/headers；(5) 新项目（bpftrace 更现代、社区更活跃）。

   </details>

3. **HFT 团队如何组合使用 BCC 和 bpftrace？**

   <details>
   <summary>参考答案</summary>

   日常排障：bpftrace one-liner 快速定位（秒级启动）。标准化监控：BCC 工具（biolatency/runqlat）做固定指标采集。深度分析：BCC + Python 后处理做复杂关联。工具开发：复杂逻辑用 BCC（可写完整 Python），简单逻辑用 bpftrace。

   </details>

</details>

---
