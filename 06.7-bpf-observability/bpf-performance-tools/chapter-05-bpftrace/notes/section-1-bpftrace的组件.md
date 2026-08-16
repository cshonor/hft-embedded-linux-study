# 5.1 bpftrace 的组件

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.1 节（印刷 p137–138）

## 内容详解

bpftrace 是**基于 BPF 和 BCC 的开源跟踪器**：自带许多性能工具和文档，同时提供一门**高级编程语言**，用于创建强大的单行程序和小工具。开篇示例：

```bash
# 以直方图统计 vfs_read() 的返回值（读取字节数或错误码）
bpftrace -e 'kretprobe:vfs_read { @bytes = hist(retval); }'
```

输出即 2 的幂次直方图——一行抵 BCC 数十行。

### 图 5-1：仓库高层结构

```
bpftrace/
├── tools/        # 工具：*.bt 脚本 + *example.txt 示例
├── man/man8/     # 每工具一份 man 8 手册
├── docs/         # 参考手册（reference_guide.md）、单行程序指引（tutorial_one_liners.md）
└── src/
    └── ast/      # 前端：lex/yacc 词法语法分析 → AST → 代码生成 LLVM IR
```

| 组件 | 内容 |
|------|------|
| `tools/*.bt` | 全部工具以 **`.bt` 后缀**命名 |
| `man/man8/*.8` | 每工具手册页 |
| `tools/*_example.txt` | 示例输出与解释 |
| `docs/reference_guide.md` | bpftrace 语言参考手册（持续更新） |
| `docs/tutorial_one_liners.md` | 新手单行程序指引 |
| `src/ast/` | 前端 lex/yacc + Clang 解析结构体；后端编译 LLVM IR → BPF |

### 与 BCC 的分工（本章学习目标）

- **bpftrace 适合**：临时创造单行程序和短小脚本进行观测；
- **BCC 适合**：编写复杂工具和守护进程；
- 学习目标包括：了解特性并与其他工具对比、找到工具和文档、读懂后续章节的 bpftrace 工具源码、自己写单行程序和工具、（可选）了解内部实现。

### 历史

Alastair Robertson 于 2016 年 12 月创建（业余项目）；因设计良好、与 BCC/LLVM/BPF 工具链匹配好，作者（Gregg）加入并成为代码/工具/文档主要贡献者；第一版主要特性 2018 年完成。

## HFT 关联

- bpftrace = **排障假设的快速验证器**：一行命令验证一个猜想（见 5.5 与附录 A）；
- 交易机上往往更适合带 bpftrace 而非完整 BCC（无 Python 依赖时），但注意它同样需要 root。

## 陷阱

- ⚠️ 工具文件后缀是 `.bt`，运行方式 `bpftrace tool.bt` 或 `./tool.bt`（需 shebang + chmod）。

<details>
<summary>自测题</summary>

1. bpftrace 与 BCC 的定位分工是什么？
   <details><summary>答案</summary>bpftrace 适合临时单行程序和短小脚本；BCC 适合复杂工具和守护进程。</details>

2. bpftrace 仓库中 tools/ 下的工具文件用什么后缀？
   <details><summary>答案</summary>`.bt`。</details>
</details>
