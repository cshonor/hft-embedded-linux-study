# 8.1 并发 bug 的类型

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### 并发 bug 分类

| 类型 | 描述 | 后果 | 检测工具 |
|------|------|------|---------|
| **死锁** (Deadlock) | 线程互相等待锁 | 系统挂死 | LOCKDEP |
| **活锁** (Livelock) | 线程不断重试但无法前进 | CPU 100% 但无进展 | 观察 |
| **数据竞争** (Race) | 无同步的并发访问 | 数据损坏 | KCSAN |
| **优先级反转** | 低优先级持锁阻塞高优先级 | 延迟飙升 | 观察 |

### 死锁的四种类型

1. **AA 死锁**: 同一线程重复获取同一锁（非递归锁）
2. **AB-BA 死锁**: 线程1 先A后B，线程2 先B后A
3. **AB-CA 死锁**: 线程1 A→B，线程2 B→C，线程3 C→A（环）
4. **自锁与中断**: 进程上下文获取锁，中断上下文也需要该锁

### HFT 关联

HFT 自定义内核模块最容易遇到：AA 死锁（回调函数中重复获取同一锁）和中断-进程死锁。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** AA 死锁和 AB-BA 死锁哪个更容易被 LOCKDEP 检测？

> 两者都能被 LOCKDEP 检测。AA 死锁在第二次获取同一锁时立即报告（即使没有实际死锁）。AB-BA 死锁在 LOCKDEP 检测到锁序矛盾时报告（需要两个线程实际执行了相反顺序的获取操作）。AA 更快被发现，因为不需要等待实际并发。


**Q:** 内核并发 bug 的两大类是什么？分别用什么工具检测？

> (1) 死锁（deadlock）：AB-BA 锁序反序 → LOCKDEP。(2) 竞态（race）：缺乏同步的并发访问 → KCSAN。另外 LOCKDEP 也检测锁上下文错误（如 spinlock 中睡眠）。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/section-8-2.md)
- [05.6 ch08 KCSAN](chapter-08-lock-debug/notes/section-8-5.md)
