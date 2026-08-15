## 2. 中断与异常的区别

---

### 一、中断 (Interrupts)

**异步**产生，通常来自 CPU **外部**硬件：

| 类型 | 来源 | 说明 |
|------|------|------|
| **可屏蔽中断** | I/O 设备 IRQ | 可通过 `cli` 等暂时屏蔽 |
| **不可屏蔽中断 (NMI)** | 严重硬件故障 | 不可屏蔽，最高优先级 |

典型：网卡收包、定时器 tick、键盘。

---

### 二、异常 (Exceptions)

**同步**产生 — CPU 执行指令时检测到：

| 子类 | 特点 | 例子 |
|------|------|------|
| **故障 (Faults)** | 常可纠正，纠正后**无缝继续** | **缺页异常**（分配物理页） |
| **陷阱 (Traps)** | 在触发指令**之后**报告 | 调试断点 |
| **异常终止 (Aborts)** | 严重错误，只能**强制终止** | 硬件严重故障 |
| **编程异常** | 程序员主动触发 | `int $0x80` → **系统调用**（软件中断） |

→ 缺页与 COW：[Ch 2](../../chapter-02-memory-addressing/) · [Ch 9](../../chapter-09-process-address-space.md)  
→ syscall：[Ch 10](../../chapter-10-system-calls.md)

---

### 三、一句话对比

| | 中断 | 异常 |
|---|------|------|
| 时机 | 异步、与当前指令无关 | 同步、与当前指令相关 |
| 来源 |  mostly 外设 | CPU 检测 / 主动 `int` |
| 典型 | 网卡 IRQ | 缺页、除零、`syscall` |

### 常见陷阱

1. 混淆 fault/trap/abort——fault 可恢复（缺页），trap 用于调试（int3），abort 不可恢复（double fault）
2. 以为所有异常都有 error code——只有部分异常推送 error code（如 page fault 推送 CR2），`int3`/`overflow` 不推送
3. 把 ULK 的 32 位异常号和 64 位混淆——64 位异常号分配有调整，且增加了 IST（Interrupt Stack Table）机制

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** Fault、Trap、Abort 三类异常的区别和典型例子？

<details><summary>答案</summary>

Fault：可恢复，CPU 恢复到触发指令重新执行（如 #PF 缺页、#GP 段错误）。Trap：调试用，CPU 恢复到下一条指令（如 #DB 断点、`int3`）。Abort：不可恢复，通常 panic（如 #DF double fault、#MC machine check）。HFT 中 #PF 在热路径上是大忌（微秒级延迟尖峰）。

</details>

**Q2.** x86-64 异常处理相比 32 位有什么新机制？

<details><summary>答案</summary>

① IST（Interrupt Stack Table）：某些关键异常（#DF, #NMI, #MC）切换到专用内核栈，避免栈溢出导致二次异常。② `IDTENTRY` 宏自动处理 error code 和栈切换。③ syscall 指令不走 IDT，直接从 MSR 加载入口。④ 64 位下 #SS（stack segment fault）基本不会触发。

</details>

**Q3.** HFT 如何检测热路径上的异常（如 page fault）？

<details><summary>答案</summary>

① `perf stat -e page-faults` 统计缺页次数。② `bpftrace -e 'tracepoint:exceptions:page_fault_user { @[comm] = count(); }'` 按进程统计。③ `/proc/[pid]/stat` 的 `minflt`（minor fault）和 `majflt`（major fault）字段。④ HFT 应确保热路径 `minflt = 0`（预分配 + 大页 + mlock）。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. IDT 与门描述符](./section-3-IDT与门描述符.md)
> ↔ [LKD Ch07 §7.1 中断的概念](../../../05-linux-kernel/chapter-07-interrupts/notes/section-7.1-中断的概念.md)
