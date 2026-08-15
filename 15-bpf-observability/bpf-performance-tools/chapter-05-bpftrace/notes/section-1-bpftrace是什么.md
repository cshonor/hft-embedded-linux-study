# 1. bpftrace 是什么

| 对比 | **BCC** | **bpftrace** |
|------|---------|--------------|
| **典型形态** | Python + 嵌入 BPF C、70+ 预置工具 | **一门脚本语言** + CLI |
| **上手成本** | 读 man、理解工具参数 | 单行即可开测 |
| **适合** | 生产 runbook、复杂多探针工具 | **ad hoc**、假设验证、教学演示 |
| **本书** | [Ch 4](../../chapter-04-bcc/) | **本章** |

**全书分工：** Ch 3 清单 → Ch 4 BCC 工具箱 → **本章语言** → Ch 6+ 按资源域（CPU/内存/网络…）展开 **具体场景与工具**。


### 常见陷阱

1. **把 bpftrace 当作编程语言** — bpftrace 是声明式追踪 DSL，不是通用编程语言；没有复杂控制流、没有函数定义、没有面向对象——追求简洁而非完备
2. **忽视 bpftrace 的安装依赖** — bpftrace 依赖 libbpf、bcc 库（部分版本）、BTF 支持；某些发行版需手动安装或从源码编译
3. **以为 bpftrace 不需要 root** — bpftrace 需要 root 或 CAP_BPF/CAP_PERFMON；普通用户无法加载 BPF 程序

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 是什么？它的设计理念是什么？**

   <details>
   <summary>参考答案</summary>

   bpftrace 是基于 BPF 的高级追踪语言（DSL），灵感来自 awk 和 DTrace。设计理念：用最简洁的语法描述「探针 + 过滤 + 动作」，让分析师专注观测逻辑而非底层工程。一行命令即可完成传统工具数十行代码的工作。

   </details>

2. **bpftrace 的基本语法结构是什么？**

   <details>
   <summary>参考答案</summary>

   `probe /filter/ { actions }`。例如 `kprobe:vfs_read /pid == 1234/ { @count++; }`。probe 是事件触发点，filter 是可选条件，actions 是命中时执行的语句。多个 probe 可在一个脚本中定义。

   </details>

3. **bpftrace 相比 BCC 的最大优势是什么？**

   <details>
   <summary>参考答案</summary>

   简洁性——bpftrace one-liner 可以替代 BCC 数十行 Python+C 代码。例如统计函数调用次数：bpftrace `kprobe:do_sys_open { @++ }` vs BCC 需要写 Python 前端 + C BPF 程序 + Map 定义。代价是灵活性不如 BCC。

   </details>

</details>

---
