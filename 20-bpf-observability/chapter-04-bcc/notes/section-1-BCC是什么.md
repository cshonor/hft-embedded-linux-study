# 1. BCC 是什么

| 维度 | 说明 |
|------|------|
| **定位** | 构建 BPF 软件的 **开源编译器框架 + 工具集** |
| **前端语言** | Python、C++、Lua |
| **编译链** | **Clang/LLVM** 将 BPF C 编译为字节码 → `bpf()` 注入内核 |
| **规模** | **70+** 单用途工具 + 若干多用途「瑞士军刀」 |

**与全书关系：** [Ch 2](../../chapter-02-technology-background/) 讲 VM/Map/探针原理；[Ch 3](../../chapter-03-performance-analysis/) 给 BCC 工具 **检查清单**；本章讲 **BCC 生态本身** 与四大多用途工具；[Ch 5](../../chapter-05-bpftrace/) 讲更轻量的 **bpftrace** 脚本语言。


### 常见陷阱

1. **把 BCC 当作单一工具** — BCC 是框架（Python 前端 + C BPF 后端 + libbpf），包含数十个预置工具；新手常以为 BCC 就是一个命令
2. **忽视 BCC 对内核头文件的依赖** — BCC 运行时编译 BPF C 代码需要内核头文件（kernel-devel/kernel-headers）；缺少头文件会报编译错误
3. **在容器中使用 BCC 不做特殊配置** — 容器需要 CAP_SYS_ADMIN/CAP_BPF + 挂载 debugfs/tracefs；普通容器默认无法运行 BCC 工具

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 的架构由哪几部分组成？**

   <details>
   <summary>参考答案</summary>

   (1) Python 前端：用户交互、参数解析、结果输出；(2) Clang/LLVM 运行时编译器：把 BPF C 编译为字节码；(3) libbpf：加载 BPF 程序、创建 Map；(4) 内核 BPF 子系统：验证、JIT、执行。开发者写 Python + C，BCC 负责编译加载。

   </details>

2. **BCC 运行时编译需要什么依赖？为什么？**

   <details>
   <summary>参考答案</summary>

   需要内核头文件包（kernel-devel 或 linux-headers），因为 BCC 在运行时用 Clang 编译 BPF C 代码，需要内核头文件中的类型定义和宏。缺少头文件会报 `fatal error: linux/xxx.h: No such file` 等编译错误。

   </details>

3. **在容器中运行 BCC 工具需要哪些特殊配置？**

   <details>
   <summary>参考答案</summary>

   (1) 以 privileged 模式运行或添加 CAP_SYS_ADMIN/CAP_BPF；(2) 挂载 debugfs 和 tracefs（`-v /sys/kernel/debug:/sys/kernel/debug`）；(3) 容器内安装内核头文件匹配宿主内核版本；(4) 确认宿主内核支持 BPF（4.x+）。

   </details>

</details>

---
