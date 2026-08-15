## 4. 进入与退出系统调用

> x86 上两种陷入内核的方式 — **慢路径** vs **快路径**

---

### 一、`int $0x80`（传统方式）

| 步骤 | 说明 |
|------|------|
| 触发 | 用户态执行 **`int $0x80`** — 向量 **128 (0x80)** 的编程异常 |
| 入口 | 跳转到 **`system_call()`** 处理程序（经 IDT） |
| 退出 | 检查 `thread_info` 标志 → 可能 **调度** / **处理信号** → **`iret`** 回用户态 |

相关标志（与 [Ch 7](../../chapter-07-process-scheduling/) 衔接）：

- **`TIF_NEED_RESCHED`** — 返回前调用 `schedule()`  
- **`TIF_SIGPENDING`** — 返回前处理信号（→ [Ch 11](../../chapter-11-signals/)）

→ IDT / 异常框架：[Ch 4](../../chapter-04-interrupts-and-exceptions/)

---

### 二、`sysenter` / `sysexit`（快速路径）

较新 Pentium 引入，**绕过 IDT 查表**：

| 组件 | 作用 |
|------|------|
| **MSR** | `SYSENTER_CS_MSR`、`SYSENTER_EIP_MSR` 等 — 直接加载内核 CS / 入口 EIP |
| **`sysenter`** | 用户态快速进入内核 |
| **`sysexit`** | 快速返回用户态 |
| **vsyscall 页** | 内核映射的特殊页，用户 libc 从此获取 fast syscall 桩代码 |

**优势：** 更少 CPU 周期 — HFT 热路径 syscall 累积开销显著。

> **Modern 对照：** 64 位 Linux 用 **`syscall`/`sysret`** + **vDSO**（如 `clock_gettime` 可无 syscall）；ULK 2.6 的 sysenter 是同一演进线的 32 位优化。

---

### 三、返回路径概览

```
system_call() 入口
    ↓
SAVE_ALL、分派 sys_*
    ↓
准备返回值 → eax
    ↓
exit_work: TIF_NEED_RESCHED? → schedule()
           TIF_SIGPENDING?  → 信号处理
    ↓
iret / sysexit → 用户态
```

→ 中断返回详述：[Ch 4 section-8](../../chapter-04-interrupts-and-exceptions/notes/section-8-中断返回.md)

### 常见陷阱

1. 把 ULK 的 `int $0x80` 入口栈帧当现代版——x86-64 `syscall` 指令只存 RIP/FLAGS，不压段寄存器
2. 以为 syscall 自动关中断——`syscall` 指令不修改 IF 标志，中断保持开启（`sysret` 也不改 IF）
3. 混淆用户态栈和内核栈——syscall 切换到内核栈（TSS 中的 `sp0`），用户栈指针存在内核栈上

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86-64 `syscall` 指令的精确行为是什么？

<details><summary>答案</summary>

① 保存 `RIP` → `RCX`（返回地址）。② 保存 `RFLAGS` → `R11`。③ 清除 `RFLAGS` 中 `TF`/`IF` 以外的某些位（不关 IF）。④ `CS` ← `MSR_STAR[47:32]`（内核代码段）。⑤ `SS` ← `MSR_STAR[47:32]+8`（内核数据段）。⑥ `RIP` ← `MSR_LSTAR`（入口函数）。⑦ 切换到内核栈（`TSS.sp0`）。不压入错误码，不查 IDT。

</details>

**Q2.** syscall 入口 `entry_SYSCALL_64` 做了哪些操作？

<details><summary>答案</summary>

① 切换到内核栈（`swapgs` 切换 GS 基址 + 读 `TSS.sp0`）。② 压入用户 `RIP`（`RCX`）、`RFLAGS`（`R11`）到内核栈（`pt_regs` 结构）。③ 压入其他寄存器（`pt_regs` 完整保存）。④ 检查 `syscall_nr` 范围。⑤ 调 `sys_call_table[syscall_nr]`。⑥ 返回时 `sysret` 指令恢复寄存器 + 切回用户栈 + `swapgs`。现代内核还加了 spectre/meltdown 缓解（KPTI 页表切换）。

</details>

**Q3.** KPTI（Kernel Page Table Isolation）对 syscall 延迟有什么影响？

<details><summary>答案</summary>

KPTI 在每次 syscall/IRQ 进入内核时切换页表（用户态页表不映射内核地址），防 Meltdown 侧信道攻击。代价：① 每次 syscall 额外一次 `CR3` 写入 + TLB flush（部分）。② syscall 延迟增加 ~100-300ns。缓解：PCID (Process Context ID) 减少 TLB flush。`nopti` 启动参数可禁用（安全风险）。HFT 在受控环境可 `nopti` + `nospectre_v2` 换取性能。

</details>

</details>

---

← [3. 分派表](./section-3-分派表与服务例程.md) · 下一节 [5. 参数传递](./section-5-参数传递.md)
