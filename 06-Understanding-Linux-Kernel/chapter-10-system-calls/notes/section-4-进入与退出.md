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

---

← [3. 分派表](./section-3-分派表与服务例程.md) · 下一节 [5. 参数传递](./section-5-参数传递.md)
