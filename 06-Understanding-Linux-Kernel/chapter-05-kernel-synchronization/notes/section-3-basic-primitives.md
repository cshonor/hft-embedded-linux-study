## 3. 基础同步原语

---

### 一、每 CPU 变量 (Per-CPU variables)

- 在 **每个 CPU 上复制一份** 数据结构  
- 本 CPU 只访问自己的副本 → **避免跨核锁**  
- 适合统计计数、per-CPU 缓存等

---

### 二、原子操作 (Atomic operations)

- **读-修改-写** 在 **单条指令**（或不可分割序列）内完成  
- 防止操作中途被中断打断  
- 引用计数、位图等的基础

---

### 三、内存屏障 (Memory barriers)

| 问题 | 屏障作用 |
|------|----------|
| 编译器重排 | 限制编译器优化顺序 |
| CPU/Store buffer 重排 | 保证 **内存操作可见顺序** |

SMP 下锁实现、无锁算法都依赖屏障语义。

---

### 四、本地中断禁用 (Local IRQ disabling)

- **`local_irq_disable()`** — 当前 CPU 上禁止 **可屏蔽中断**  
- 常与 **自旋锁** 联用：
  - 单 CPU：防 ISR 与当前路径 **重入** 同一数据  
  - 多 CPU：还需 spinlock 防其他核并发

| 组合 | 防谁 |
|------|------|
| 仅 spinlock | 其他 CPU / 内核路径 |
| spinlock + local_irq_disable | 再加 **本核 ISR** |

→ 中断嵌套：[Ch 4](../chapter-04-interrupts-and-exceptions/)

---

← [2. 内核抢占](./section-2-kernel-preemption.md) · 下一节 [4. 自旋锁](./section-4-spinlocks.md)
