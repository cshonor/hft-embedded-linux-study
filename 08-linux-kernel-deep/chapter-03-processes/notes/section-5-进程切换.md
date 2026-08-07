## 5. 进程切换 (Process Switch)

> 挂起当前 CPU 上的进程，恢复另一个 — **上下文切换**

---

### 一、硬件上下文

进程恢复执行前，必须把 **CPU 寄存器** 恢复到被挂起时的值。

80x86 有硬件 **TSS（任务状态段）** 切换机制，但 Linux 为更高控制权和效率，**主要用软件（指令流）做上下文切换**。

---

### 二、切换两步走

| 步骤 | 组件 | 做什么 |
|------|------|--------|
| 1 | **`switch_to` 宏** | 切换内核栈 + 硬件上下文 |
| 2 | **`__switch_to()`** | 处理 LDT、杂项寄存器等底层细节 |

调度器（Ch 7）决定**切到谁**；本章讲**怎么切**。

→ 深潜：[Ch 7 进程调度](../../chapter-07-process-scheduling.md)

---

### 三、FPU/MMX/SSE 惰性保存

浮点 / SIMD 寄存器保存开销大。Linux 策略：

- **默认不保存** FPU/MMX/SSE 状态  
- **仅当**进程实际使用协处理器指令时，才 save/restore  

减少无关进程的切换成本 — HFT 里若热路径用 SIMD，需留意 **FPU 状态** 与 **内核抢占** 的交互。

---

### 四、与 Ch 2 TLB 的衔接

进程切换常伴随 **页表切换** → **TLB 刷新**（Ch 2 lazy TLB）→ 切换成本不止寄存器。

### 常见陷阱

1. 把 ULK 讲的 `switch_to()` 宏当现代版——6.x 的 `switch_to()` 是架构相关内联汇编，且加入了 spectre/meltdown 缓解
2. 以为上下文切换只保存寄存器——还要切换 `mm_struct`（`switch_mm()`）、FPU 状态、TLS 段、信号掩码
3. 混淆 `schedule()` 和 `context_switch()`——`schedule()` 选下一个进程，`context_switch()` 执行切换

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `context_switch()` 的两个核心步骤是什么？

<details><summary>答案</summary>

① `switch_mm()`（或 `activate_mm()`）：切换 `mm_struct`，加载新页表（`CR3` 寄存器写入），刷新 TLB（如有必要）。② `switch_to()`：保存当前寄存器到 `thread_struct`，恢复新进程寄存器，跳转到新进程的返回点。`switch_mm()` 开销远大于 `switch_to()`（页表切换 + TLB 刷新）。

</details>

**Q2.** 为什么内核线程切换不需要 `switch_mm()`？

<details><summary>答案</summary>

内核线程 `mm = NULL`，不拥有用户地址空间。切换到内核线程时，`active_mm` 保留前一个用户进程的 `mm`，不写 `CR3`，不刷 TLB。这就是「lazy TLB」——内核线程借用前一个进程的地址空间映射，避免昂贵的 TLB 刷新。

</details>

**Q3.** HFT 中 context switch 的实际开销是多少？如何测量？

<details><summary>答案</summary>

单核切换：~1-3 us（含 `switch_mm`），纯 `switch_to`：~100-300 ns。测量：`perf bench sched messaging` 或 `context_switch` 微基准。HFT 减少切换的方法：① 绑核 + `isolcpus` 消除抢占调度；② `SCHED_FIFO` 避免被普通进程抢占；③ 减少 system call（每次 syscall 返回可能触发调度）。

</details>

</details>

---

← [4. 组织与查找](./section-4-组织与查找.md) · 下一节 [6. 创建与销毁](./section-6-创建与销毁.md)
