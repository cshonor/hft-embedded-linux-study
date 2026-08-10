# 1.2 经典 Bug 案例

> ⬜ 跳读

## 本节要点

| 案例 | 教训 |
|------|------|
| Patriot 导弹 (1991) | 定点数累积误差，28 人丧生 |
| Ariane 5 (1996) | 64-bit float → 16-bit int 溢出，5 亿美元损失 |
| Mars Pathfinder (1997) | 优先级反转导致系统重启 |
| Boeing 737 MAX (2018) | 单点故障 + 软件设计缺陷 |

## HFT 关联

- Ariane 5 案例：类型转换溢出在 HFT 交易系统中同样致命
- Mars Pathfinder：优先级反转是 HFT 系统的典型陷阱（交易线程被低优先级线程阻塞）
- 教训：边界条件测试和并发设计验证至关重要

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Mars Pathfinder 的优先级反转问题是什么？如何解决的？

> 低优先级气象线程持有共享锁，高优先级总线管理线程等待该锁，中优先级线程抢占低优先级线程导致高优先级线程间接被阻塞。解决方法：启用优先级继承 (priority inheritance)，低优先级线程在持有锁时临时继承等待者的优先级。


**Q:** 什么是 "Heisenbug"？内核调试中为什么常见？

> Heisenbug 是指在调试模式下消失或行为改变的 bug。内核中常见因为：加调试选项改变内存布局/时序、printk 改变时序导致竞态消失、KASAN 的 redzone 掩盖越界。应对策略：用 ftrace（开销小）替代 printk，用 KCOV 做覆盖率引导模糊测试。

**Q:** 经典内核 bug 案例（如 Linux 2.6.37 的 VFS 死锁）给调试方法演进带来了什么启示？

> 复杂死锁难以通过代码审查发现，催生了 LOCKDEP 自动锁依赖检测。这类 bug 说明：人工分析锁依赖在大规模代码中不可行，需要运行时自动构建依赖图。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/section-8-2.md)
