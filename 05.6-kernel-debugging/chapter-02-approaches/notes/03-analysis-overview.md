# 2.3 分析方法概览

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

分析方法 (Analysis) 不在运行时观察程序，而是从源码、编译输出、崩溃转储中推断问题。

## 分析方法分类

| 方法 | 工具 | 说明 | 何时使用 |
|------|------|------|---------|
| 代码审查 | grep/cscope/ctags | 源码静态阅读 | 理解代码逻辑 |
| Oops 分析 | addr2line/objdump | 崩溃后分析 | 内核崩溃时 |
| 静态分析 | Sparse/Smatch/Coccinelle | 编译时检查 | 提交前检查 |
| 代码覆盖率 | GCOV/KCOV | 测试完整性 | 测试质量评估 |
| 模糊测试 | syzkaller | 自动发现 bug | 回归测试 |
| 二进制分析 | readelf/nm/objdump | ELF 文件分析 | 符号/重定位问题 |

## 分析 vs 仪表化

| 维度 | 分析 (Analysis) | 仪表化 (Instrumentation) |
|------|----------------|------------------------|
| 时机 | 编译时/崩溃后 | 运行时 |
| 侵入性 | 无 | 有（改变时序/性能） |
| 覆盖面 | 所有代码路径 | 仅执行到的路径 |
| 精确度 | 潜在问题 | 确认的问题 |
| 误报率 | 较高 | 零 |
| 代表工具 | Sparse/Smatch/addr2line | ftrace/KASAN/KGDB |

**实际调试中两者结合**：先分析确定范围 → 再仪表化验证假设 → 修复后分析确认

## 代码审查工具

```bash
# cscope: 内核源码交叉引用
make ARCH=arm64 cscope
# 在 vim 中使用:
#   :cs find g symbol_name    " 查找定义
#   :cs find c symbol_name    " 查找调用者
#   :cs find s symbol_name    " 查找引用

# grep: 快速搜索
grep -rn "spin_lock.*mutex" drivers/  # 查找潜在锁问题
grep -rn "GFP_KERNEL" drivers/ | grep -i "irq"  # 中断中睡眠？

# ctags: 符号跳转
make ARCH=arm64 tags
# 在 vim 中: Ctrl-] 跳转定义, Ctrl-T 返回
```

## Oops 分析流程

```
1. 收集 Oops 日志
   └── dmesg > oops.log 或 /var/log/kmsg

2. 提取关键信息
   ├── PC (程序计数器) → 崩溃地址
   ├── Call Trace → 调用栈
   ├── Register dump → 寄存器状态
   └── Code dump → 崩溃指令

3. 定位源码
   ├── addr2line -e vmlinux <addr> → 源码文件:行号
   ├── objdump -d vmlinux → 反汇编
   └── faddr2line vmlinux <func>+<offset>

4. 分析根因
   ├── 查看源码逻辑
   ├── 检查数据结构状态
   └── 推断 bug 类型（空指针/越界/UAF）
```

## 静态分析工具

| 工具 | 检查内容 | 误报率 | 内核集成 |
|------|---------|--------|---------|
| Sparse | 类型限定符、地址空间、RCU | 中 | ✅ `make C=1` |
| Smatch | 空指针、锁不平衡、越界 | 中 | ✅ `make CHECK=sparse` |
| Coccinelle | 模式匹配、API 迁移 | 低 | ✅ `scripts/coccinelle/` |
| GCC static analyzer | 多种 | 高 | ⚠️ 需 GCC 10+ |

```bash
# Sparse: 编译时检查
make C=1 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- drivers/my_driver.o

# Smatch: 更强的静态分析
smatch_scripts/build_kernel_data.sh
smatch --project=kernel drivers/my_driver.c

# Coccinelle: 模式匹配
make coccicheck MODE=patch COCCI=scripts/coccinelle/null/deref.cocci
```

## HFT 关联

HFT 内核模块的分析策略：

1. **提交前**：Sparse + Smatch 静态检查 + checkpatch.pl
2. **崩溃后**：Oops 日志 → addr2line → 源码定位
3. **测试覆盖**：KCOV 收集覆盖率，syzkaller 模糊测试
4. **代码审查**：cscope 理解调用链，grep 检查常见错误模式

```bash
# HFT 驱动常见检查模式
# 检查中断上下文中是否调用了可能睡眠的函数
grep -rn "GFP_KERNEL\|mutex_lock\|copy_from_user" drivers/my_hft/ | grep -i "irq\|spin"

# 检查错误路径是否正确释放资源
grep -A5 "goto.*out\|goto.*err\|goto.*fail" drivers/my_hft/*.c
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 静态分析工具 (Sparse/Smatch) 和运行时检测 (KASAN) 的区别？

> 静态分析在编译时检查源码，不需要运行程序，能发现潜在问题但误报率高。运行时检测在程序执行时检查实际行为，零误报但只能发现被触发路径上的问题。两者互补：静态分析覆盖面广，运行时检测精确。

**Q2:** 静态分析和动态分析在内核调试中如何配合？

> 静态分析（Sparse/Smatch/Coccinelle）在编译期发现潜在 bug（空指针、锁不平衡）。动态分析（KASAN/LOCKDEP/KCSAN）在运行时发现实际触发的 bug。两者互补：静态分析覆盖所有代码路径但不确认是否触发，动态分析确认触发但只覆盖执行到的路径。

**Q3:** cscope 和 grep 在代码审查中各有什么优劣？

> cscope 构建符号索引数据库，查找定义/调用者/引用速度快且准确，适合理解调用链。grep 是通用文本搜索，不需要建索引，适合快速搜索特定模式（如 "GFP_KERNEL"）。cscope 更精确但需要预构建，grep 更灵活但可能误匹配。

**Q4:** Coccinelle 相比 Sparse 有什么优势？

> Coccinelle 基于语义模式匹配（Semantic Patch Language, SmPL），可以描述复杂的代码模式（如 "持有自旋锁时调用 mutex_lock"）。Sparse 主要检查类型限定符和简单规则。Coccinelle 更灵活、误报更低，可以自动生成修复补丁。

**Q5:** Oops 分析中 addr2line 输出 "??" 时该怎么办？

> 可能原因：(1) vmlinux 没有编译 DEBUG_INFO；(2) 编译器优化导致内联；(3) 地址属于模块但用了 vmlinux。解决：确保 CONFIG_DEBUG_INFO=y，模块用 .ko 文件，用 faddr2line 脚本替代（支持内联展开），或用 objdump 手动反汇编分析。

</details>

## 交叉引用

- [05.6 ch07 Oops 分析](chapter-07-oops/notes/01-oops-vs-panic.md)
- [05.6 ch12 静态分析](chapter-12-misc/notes/04-static-analysis-sparse-smatch.md)
- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/02-kasan.md)
