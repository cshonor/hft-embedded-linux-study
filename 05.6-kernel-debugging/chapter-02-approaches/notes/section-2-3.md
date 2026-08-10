# 2.3 分析方法概览

> ⬜ 跳读

## 本节要点

| 方法 | 工具 | 说明 |
|------|------|------|
| 代码审查 | grep, cscope | 源码静态分析 |
| Oops 分析 | addr2line, objdump | 崩溃后分析 |
| 静态分析 | Sparse, Smatch | 编译时检查 |
| 代码覆盖率 | GCOV, KCOV | 测试完整性 |
| 模糊测试 | syzkaller | 自动发现 bug |

## 分析 vs 仪表化

- **仪表化** (instrumentation)：在运行时观察程序行为
- **分析** (analysis)：从源码/日志/崩溃转储推断问题

实际调试中两者结合：先分析确定范围，再仪表化验证假设。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 静态分析工具 (Sparse/Smatch) 和运行时检测 (KASAN) 的区别？

> 静态分析在编译时检查源码，不需要运行程序，能发现潜在问题但误报率高。运行时检测在程序执行时检查实际行为，零误报但只能发现被触发路径上的问题。两者互补：静态分析覆盖面广，运行时检测精确。


**Q:** 静态分析和动态分析在内核调试中如何配合？

> 静态分析（Sparse/Smatch/Coccinelle）在编译期发现潜在 bug（空指针、锁不平衡）。动态分析（KASAN/LOCKDEP/KCSAN）在运行时发现实际触发的 bug。两者互补：静态分析覆盖所有代码路径但不确认是否触发，动态分析确认触发但只覆盖执行到的路径。

</details>

## 交叉引用

- [05.6 ch12 Sparse/Smatch](chapter-12-misc/notes/section-12-4.md)
- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/section-5-2.md)
