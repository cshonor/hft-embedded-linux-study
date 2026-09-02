## ③ 系统调用处理程序 · Handler & Parameters

> 承接 [5.2 系统调用基础](./section-5.2-系统调用基础.md)。
> 本节回答：**陷入内核之后，第一个执行的通用函数 `do_syscall_64` 到底做了什么。**

重点：**陷入内核之后，第一步执行的通用入口函数。**
用户态 **不能直接执行内核代码** — 必须 **陷入（trap）** 切到内核态。

---

### 1. 全景：从 `syscall` 指令到 `sys_*`

```
用户态：glibc 包装函数（read/write/open...）
    │  rax = 号码, rdi/rsi/rdx/r10/r8/r9 = 参数
    ▼
syscall 指令
    │  CPU: 切特权级、保存 RIP→rcx、RFLAGS→r11、跳 MSR_LSTAR
    ▼
entry_SYSCALL_64（汇编，arch/x86/entry/entry_64.S:87）
    │  swapgs → 换 CR3 → 换内核栈 → 构造 pt_regs
    │  → PUSH_AND_CLEAR_REGS → IBRS_ENTER → UNTRAIN_RET
    ▼
do_syscall_64（arch/x86/entry/common.c:73）          ← 本节主角
    │
    ├─ add_random_kstack_offset()            内核栈随机偏移
    ├─ syscall_enter_from_user_mode(regs, nr)
    │      └─ User Dispatch → ptrace → seccomp → 重读号码 → tracepoint → audit
    ├─ do_syscall_x64(regs, nr)              或 do_syscall_x32(regs, nr)
    │      └─ array_index_nospec(unr, NR_syscalls)   ← Spectre v1 防护
    │      └─ regs->ax = sys_call_table[unr](regs)
    └─ syscall_exit_to_user_mode(regs)
           └─ 信号投递 → 任务抢占 → 抽下一个栈随机偏移
    ▼
sysretq
```

> ⭐ 注意中间那个 `syscall_enter_from_user_mode()` 是 **架构无关**的一层（`kernel/entry/common.c`），x86 只是它众多调用者之一。这是 v5.12 之后的重大重构——把 syscall 入口的"通用工作"（ptrace / seccomp / audit）从各架构汇编里抽了出来。

---

### 2. ⭐ `do_syscall_64`：v6.6 真实源码

`arch/x86/entry/common.c:73-87` 全文：

```c
__visible noinstr void do_syscall_64(struct pt_regs *regs, int nr)
{
	add_random_kstack_offset();
	nr = syscall_enter_from_user_mode(regs, nr);

	instrumentation_begin();

	if (!do_syscall_x64(regs, nr) && !do_syscall_x32(regs, nr) && nr != -1) {
		/* Invalid system call, but still a system call. */
		regs->ax = __x64_sys_ni_syscall(regs);
	}

	instrumentation_end();
	syscall_exit_to_user_mode(regs);
}
```

| 行 | 作用 |
|----|------|
| `__visible` | 防止编译器认为无人引用而优化掉（因为只有汇编调用它） |
| `noinstr` | ⭐ **禁止插桩**——这段代码处在 Spectre 缓解的关键窗口，不能有 ftrace/kprobe 探针打断 |
| `add_random_kstack_offset()` | ⭐ 内核栈随机偏移（见 §7） |
| `syscall_enter_from_user_mode()` | ⭐ 架构无关进入层，**返回值可能改写 `nr`**（见 §6） |
| `instrumentation_begin/end()` | 只有中间这段允许被插桩 |
| `nr != -1` | ⭐ `-1` 表示"这不是一个真的 syscall，别执行" |
| 注释 *"Invalid system call, but still a system call."* | ⭐ 即使号码非法，audit/seccomp/tracepoint 已经记录过它了，所以仍要算作"一次 syscall" |
| `syscall_exit_to_user_mode()` | ⭐ 架构无关退出层：信号投递、抢占、栈偏移轮换 |

#### ⭐ `nr == -1` 从哪来？

进入层返回 `-1L` 有三种情形：

| 来源 | 含义 |
|------|------|
| `syscall_user_dispatch()` 拦下 | Syscall User Dispatch（Wine/Proton 模拟 Windows 二进制） |
| `ptrace_report_syscall_entry()` 返回非 0，或开了 `SYSCALL_EMU` | tracer 决定不执行 |
| `__secure_computing()` 返回 `-1L` | seccomp BPF 判定为**拒绝** |

这三种情况下**内核函数一个都不执行**，但 `regs->ax` 已被设置成 seccomp/ptrace 指定的返回值。

---

### 3. ⭐ `do_syscall_x64`：分派的核心，只有 6 行

```c
static __always_inline bool do_syscall_x64(struct pt_regs *regs, int nr)
{
	/*
	 * Convert negative numbers to very high and thus out of range
	 * numbers for comparisons.
	 */
	unsigned int unr = nr;

	if (likely(unr < NR_syscalls)) {
		unr = array_index_nospec(unr, NR_syscalls);
		regs->ax = sys_call_table[unr](regs);
		return true;
	}
	return false;
}
```

#### 三个细节

**① `unsigned int unr = nr` —— 无分支的负数处理**

注释说得很清楚："Convert negative numbers to very high and thus out of range numbers"。
`int nr = -1` → `unsigned int unr = 0xFFFFFFFF` → `unr < NR_syscalls` 为假 → 直接返回 false。

**省掉了一次 `nr >= 0` 的显式判断**。

**② ⭐ `array_index_nospec()` —— Spectre v1 防护**

这是最容易被忽略的一点。`array_index_nospec(unr, NR_syscalls)` 的作用是**阻止 CPU 推测执行时用越界的 `unr` 去读 `sys_call_table`**。

原理：它把索引"钳制"到合法范围内，即使分支预测错误，推测路径读到的也永远是表内的地址，攻击者无法通过 cache 侧信道探测表外数据。

> ⭐ 这也是[5.2 讲过的 syscall 开销](./section-5.2-系统调用基础.md)的一部分——**每一次 syscall 都要付一次 Spectre v1 的缓解成本**。

**③ ⭐ 所有 syscall 只接受一个参数：`struct pt_regs *`**

```c
regs->ax = sys_call_table[unr](regs);
```

注意：**不是** `sys_call_table[unr](arg1, arg2, ..., arg6)`，而是把整个 `pt_regs` 丢进去。

这是现代内核的关键设计（对应 `CONFIG_ARCH_HAS_SYSCALL_WRAPPER`）：

| | 老式（i386 时代） | 现代（x86_64） |
|---|------------------|---------------|
| `sys_call_table` 元素签名 | 6 个参数，类型各异 | ⭐ **统一为 `long (*)(const struct pt_regs *)`** |
| 参数提取 | CPU 直接按 ABI 取 | 由 `SYSCALL_DEFINE` 宏生成的包装层从 `pt_regs` 里取 |
| 好处 | 直接 | ⭐ 统一处理符号扩展/零扩展、compat 层、tracepoint、参数复用 |

---

### 4. ⭐ `sys_call_table` 是怎么生成的

`arch/x86/entry/syscall_64.c` **全文只有 451 字节**：

```c
// SPDX-License-Identifier: GPL-2.0
/* System call table for x86-64. */

#include <linux/linkage.h>
#include <linux/sys.h>
#include <linux/cache.h>
#include <linux/syscalls.h>
#include <asm/syscall.h>

#define __SYSCALL(nr, sym) extern long __x64_##sym(const struct pt_regs *);
#include <asm/syscalls_64.h>
#undef __SYSCALL

#define __SYSCALL(nr, sym) __x64_##sym,

asmlinkage const sys_call_ptr_t sys_call_table[] = {
#include <asm/syscalls_64.h>
};
```

#### 四处精妙之处

| 技巧 | 说明 |
|------|------|
| ⭐ **X-macro** | 同一个 `asm/syscalls_64.h` **两次 include**，每次配不同的 `__SYSCALL` 定义：第一次生成 extern 声明，第二次生成数组元素 |
| ⭐ **`const`** | `const sys_call_ptr_t sys_call_table[]` —— 表本身只读，防止运行时被篡改（rootkit 常见攻击点） |
| ⭐ **`__x64_` 前缀** | 与 [5.2](./section-5.2-系统调用基础.md) 呼应：表里写 `sys_read`，实际符号是 `__x64_sys_read` |
| **自动生成** | `asm/syscalls_64.h` 由 `scripts/syscalltbl.sh` 从 `syscall_64.tbl` 生成，**不在源码树里** |

```
syscall_64.tbl  ──(scripts/syscalltbl.sh 构建时生成)──►  asm/syscalls_64.h
                                                            │
                                    ┌───────────────────────┘
                                    ▼
                    #define __SYSCALL(nr, sym) extern long __x64_##sym(...);
                    #include <asm/syscalls_64.h>      ← 声明
                                    │
                    #define __SYSCALL(nr, sym) __x64_##sym,
                    const sys_call_ptr_t sys_call_table[] = {
                    #include <asm/syscalls_64.h>      ← 定义
                    };
```

> ⭐ **为什么表里那 89 个空洞（335-423）不会浪费内存**：表是**连续数组**，空洞位置由生成器填 `__x64_sys_ni_syscall`，所以数组不"稀疏"。这正是 §5.2 讲的"为 x32 预留号段"所付出的代价——**数组跨度变大，但元素一个不少**。

---

### 5. ⭐ x32 分派：`__X32_SYSCALL_BIT`

`do_syscall_64` 里的第二分支：

```c
static __always_inline bool do_syscall_x32(struct pt_regs *regs, int nr)
{
	/*
	 * Adjust the starting offset of the table, and convert numbers
	 * < __X32_SYSCALL_BIT to very high and thus out of range
	 * numbers for comparisons.
	 */
	unsigned int xnr = nr - __X32_SYSCALL_BIT;

	if (IS_ENABLED(CONFIG_X86_X32_ABI) && likely(xnr < X32_NR_syscalls)) {
		xnr = array_index_nospec(xnr, X32_NR_syscalls);
		regs->ax = x32_sys_call_table[xnr](regs);
		return true;
	}
	return false;
}
```

| 事实 | 说明 |
|------|------|
| `__X32_SYSCALL_BIT` | x32 syscall 的号码 = 基础号码 **加上这个位** |
| 判定方式 | `nr - __X32_SYSCALL_BIT` 后看是否 `< X32_NR_syscalls` |
| 独立的表 | `x32_sys_call_table[]`（不是 `sys_call_table[]`） |
| 条件编译 | `IS_ENABLED(CONFIG_X86_X32_ABI)` —— 关闭时这个分支被完全消除 |

> ⭐ 这与 [Ch 1.1](../../chapter-01-intro/notes/section-1.1-Unix-的历史.md) 和 [5.2](./section-5.2-系统调用基础.md) 讲的"x32 号段 512-547"是同一件事的**汇编侧实现**：用户态传进来的 `rax` 带着 `__X32_SYSCALL_BIT` 标记，内核据此分流到 x32 表。

---

### 6. ⭐ 架构无关的进入层：7 个 `SYSCALL_WORK` 标志

`kernel/entry/common.c:50-84`（节选）：

```c
	long ret = 0;

	/*
	 * Handle Syscall User Dispatch.  This must comes first, since
	 * the ABI here can be something that doesn't make sense for
	 * other syscall_work features.
	 */
	if (work & SYSCALL_WORK_SYSCALL_USER_DISPATCH) {
		if (syscall_user_dispatch(regs))
			return -1L;
	}

	/* Handle ptrace */
	if (work & (SYSCALL_WORK_SYSCALL_TRACE | SYSCALL_WORK_SYSCALL_EMU)) {
		ret = ptrace_report_syscall_entry(regs);
		if (ret || (work & SYSCALL_WORK_SYSCALL_EMU))
			return -1L;
	}

	/* Do seccomp after ptrace, to catch any tracer changes. */
	if (work & SYSCALL_WORK_SECCOMP) {
		ret = __secure_computing(NULL);
		if (ret == -1L)
			return ret;
	}

	/* Either of the above might have changed the syscall number */
	syscall = syscall_get_nr(current, regs);

	if (unlikely(work & SYSCALL_WORK_SYSCALL_TRACEPOINT))
		trace_sys_enter(regs, syscall);

	syscall_enter_audit(regs, syscall);

	return ret ? : syscall;
```

#### ⭐⭐ 处理顺序是**有讲究**的，不是随便排的

| 顺序 | 步骤 | 为什么在这个位置 |
|------|------|----------------|
| 1 | **Syscall User Dispatch** | 注释原文：*"This must comes first, since the ABI here can be something that doesn't make sense for other syscall_work features"*（Wine 场景下的 ABI 可能不合法，先拦） |
| 2 | **ptrace** | tracer 可能修改寄存器/号码 |
| 3 | ⭐ **seccomp** | 注释原文：***"Do seccomp after ptrace, to catch any tracer changes."*** —— 必须让 seccomp 看到 tracer 改过之后的状态，否则 tracer 能绕过 seccomp 过滤 |
| 4 | ⭐ **重新读号码** | 注释原文：***"Either of the above might have changed the syscall number"*** → `syscall = syscall_get_nr(current, regs)` |
| 5 | tracepoint | 号码已定，可以记录 |
| 6 | audit | 同上 |

> ⭐⭐ **第 3 步的注释是安全关键**：如果 seccomp 在 ptrace 之前跑，那么 tracer（拥有进程控制权）就可以先被 seccomp 检查通过，然后偷偷把 syscall 号改成危险的——**seccomp 形同虚设**。所以必须 ptrace 在前、seccomp 在后。

#### 7 个标志（`include/linux/thread_info.h:42-57`）

```c
	SYSCALL_WORK_BIT_SECCOMP,
	SYSCALL_WORK_BIT_SYSCALL_TRACEPOINT,
	SYSCALL_WORK_BIT_SYSCALL_TRACE,
	SYSCALL_WORK_BIT_SYSCALL_EMU,
	SYSCALL_WORK_BIT_SYSCALL_AUDIT,
	SYSCALL_WORK_BIT_SYSCALL_USER_DISPATCH,
	SYSCALL_WORK_BIT_SYSCALL_EXIT_TRAP,
```

| 标志 | 触发者 | 作用 |
|------|--------|------|
| `SYSCALL_WORK_SECCOMP` | `seccomp(2)` / `prctl(PR_SET_SECCOMP)` | BPF 程序过滤 syscall |
| `SYSCALL_WORK_SYSCALL_TRACE` | `ptrace(PTRACE_SYSCALL)` | **strace** 用的就是这个 |
| `SYSCALL_WORK_SYSCALL_EMU` | ptrace | 只通知 tracer，**不真执行** syscall |
| `SYSCALL_WORK_SYSCALL_TRACEPOINT` | ftrace / perf / BPF | `raw_syscalls:sys_enter` tracepoint |
| `SYSCALL_WORK_SYSCALL_AUDIT` | auditd 规则 | 审计日志 |
| `SYSCALL_WORK_SYSCALL_USER_DISPATCH` | `prctl(PR_SET_SYSCALL_USER_DISPATCH)` | ⭐ Wine / Proton 模拟 Windows 二进制 |
| `SYSCALL_WORK_SYSCALL_EXIT_TRAP` | — | 退出时也陷入通知 |

标志存在 `thread_info->syscall_work` 位图里（同一头文件 `:150-161` 的 `set_bit` / `test_bit` / `clear_bit` 封装）。

#### ⭐ 为什么"没开追踪时 syscall 几乎零额外成本"

所有检查都是 `if (work & FLAG)` —— **一次位测试**。`work` 从 `current_thread_info()->syscall_work` 读，通常在 cache 里。

一个既不 strace、又没 seccomp、又没 audit 的进程，`work == 0`，六个分支全部不命中，**直接跳到分派**。

| 进程类型 | 每次 syscall 的额外成本 |
|---------|----------------------|
| 普通进程 | ⭐ **零**（一次位测试） |
| 开了 seccomp（如容器/Chrome 沙箱） | 每次跑一遍 BPF 程序 |
| 被 `strace -T` 跟踪 | ⭐ 每次 syscall **两次**停止 + 两次 tracer 上下文切换 |

> **HFT 警示**：`strace` 的开销**不是线性的**，而是每次 syscall 都要把被跟踪进程停下来、唤醒 tracer、tracer 读寄存器、再恢复。**热路径上每秒百万次 syscall 的程序，挂上 strace 会慢到完全不可测**。生产环境用 `perf` 采样或 BPF（`bpftrace`）替代。

---

### 7. ⭐ 进程内核栈：大小、随机偏移、栈溢出防护

| 栈 | 何时用 |
|----|--------|
| **用户栈** | 用户态运行 |
| **进程专属内核栈** | 一旦进入系统调用，立刻切换 |

系统调用里的局部变量、函数调用栈，全部存在 **内核栈**。

#### 大小（v6.6 `arch/x86/include/asm/page_64_types.h:15-16`）

```c
#define THREAD_SIZE_ORDER	(2 + KASAN_STACK_ORDER)
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)
```

| 配置 | `THREAD_SIZE_ORDER` | 内核栈大小 |
|------|--------------------|-----------|
| 默认 | 2 | `4096 << 2` = **16 KB** |
| 开 KASAN | 3（`KASAN_STACK_ORDER=1`） | **32 KB** |

> ⭐ **16KB 是很小的**。内核里的递归、大局部数组（如 `char buf[8192]`）都极易爆栈。这也是为什么内核编码规范**禁止大局部变量和深递归**。

#### ⭐ 栈随机偏移：为什么在**退出时**才抽下一个？

`include/linux/randomize_kstack.h`：

```c
#define KSTACK_OFFSET_MAX(x)	((x) & 0x3FF)

#define add_random_kstack_offset() do {					\
	if (static_branch_maybe(CONFIG_RANDOMIZE_KSTACK_OFFSET_DEFAULT,	\
				&randomize_kstack_offset)) {		\
		u32 offset = raw_cpu_read(kstack_offset);		\
		u8 *ptr = __kstack_alloca(KSTACK_OFFSET_MAX(offset));	\
		/* Keep allocation even after "ptr" loses scope. */	\
		asm volatile("" :: "r"(ptr) : "memory");		\
	}								\
} while (0)
```

| 事实 | 值 / 说明 |
|------|----------|
| 熵上限 | ⭐ **10 bit** = 偏移最多 **1023 字节**（`KSTACK_OFFSET_MAX` = `& 0x3FF`） |
| 实现方式 | `alloca` 在栈上"浪费"一段空间 |
| 开关 | `static_branch_maybe(...)` —— static key，运行时可切换，**关闭时零开销** |
| 为何用 `_uninitialized` 变体 | 注释：*"Initializing the unused area on each syscall entry is expensive"* |

⭐⭐ **最精彩的一段：为什么 `choose_random_kstack_offset()` 在 syscall 退出时调用，而不是进入时？**

源码注释原文：

> ```
>  * This should only be used during syscall exit when interrupts and
>  * preempt are disabled. This position in the syscall flow is done to
>  * frustrate attacks from userspace attempting to learn the next offset:
>  * - Maximize the timing uncertainty visible from userspace: if the
>  *   offset is chosen at syscall entry, userspace has much more control
>  *   over the timing between choosing offsets. "How long will we be in
>  *   kernel mode?" tends to be more difficult to predict than "how long
>  *   will we be in user mode?"
>  * - Reduce the lifetime of the new offset sitting in memory during
>  *   kernel mode execution. Exposure of "thread-local" memory content
>  *   (e.g. current, percpu, etc) tends to be easier than arbitrary
>  *   location memory exposure.
> ```

翻译成人话：

| 理由 | 说明 |
|------|------|
| ⭐ **增大计时不确定性** | 如果在**进入时**抽偏移，用户态能精确控制"抽偏移"到自己下一次 syscall 之间的时间；而在**退出时**抽，攻击者得预测"我这次进入内核会待多久"——这个难得多 |
| ⭐ **缩短新偏移在内存里的暴露时间** | 退出时抽完，紧接着就回用户态，新偏移在内核内存里停留的时间最短；而 `current` / percpu 这类"线程局部"内存比任意内存更容易被泄露 |

> 这是"用时序设计换安全性"的教科书级例子，且**不增加任何运行时开销**——只是把抽随机数的位置挪了一下。

#### 栈溢出检测

| 机制 | 说明 |
|------|------|
| `CONFIG_VMAP_STACK` | 内核栈用 `vmalloc` 分配，前后放 **guard page（不可映射页）** → 栈溢出立刻触发缺页异常而非静默写坏邻居 |
| `CONFIG_SCHED_STACK_END_CHECK` | 栈底放 canary，每次调度检查 |
| 栈溢出症状 | `BUG: stack guard page was hit` / `kernel stack overflow` |

---

### 8. 参数是怎么传给 `sys_*` 的

表项只接受 `struct pt_regs *` **这一个**参数（见 §3），那 `__x64_sys_read(unsigned int fd, char *buf, size_t count)` 的 6 个参数从哪来？

**由 `SYSCALL_DEFINE` 宏自动生成的包装层提取**（`CONFIG_ARCH_HAS_SYSCALL_WRAPPER`）：

```
sys_call_table[nr](regs)
    │
    ▼
__x64_sys_read(regs)                 ← 表里存的就是它，签名统一
    │
    ▼
__se_sys_read(fd, buf, count)        ← 从 regs->di/si/dx 取值，做符号/零扩展
    │
    ▼
__do_sys_read(fd, buf, count)        ← 你写的函数体
```

| 层 | 职责 |
|----|------|
| `__x64_sys_*` | 统一签名 `long (*)(const struct pt_regs *)`，供表调用 |
| `__se_sys_*` | **s**ign **e**xtend：从 `pt_regs` 取参数并做符号/零扩展（安全性关键） |
| `__do_sys_*` | 真正的实现 |

> ⭐ 为什么需要 `__se_sys_*` 这层？因为 C 语言的类型系统无法表达"这个 32 位参数在 64 位寄存器里"。例如 `int fd` 从 `regs->di` 取低 32 位时必须正确符号扩展，否则 `-1` 会变成 `0xFFFFFFFF`（无符号）。这层自动生成，避免手写出错。

---

### 9. 修正一处常见误传：原文这行代码是残的

很多教材（含本笔记旧版）写着：

```c
nr = regs->ax;
if (nr >= NR_syscalls)
    return -ENOSYS;
sys_call_tablenr;      /* ← 残缺行 */
```

**v6.6 的实际写法**（§3）不同之处在于：

| 教材版 | v6.6 实际 |
|--------|----------|
| `if (nr >= NR_syscalls) return -ENOSYS;` | `if (likely(unr < NR_syscalls)) { ... } return false;`（**正数判断**，`likely` 提示分支预测） |
| 无 | ⭐ `array_index_nospec(unr, NR_syscalls)`（**Spectre v1 防护**） |
| `return -ENOSYS` | `regs->ax = __x64_sys_ni_syscall(regs)`（经 `do_syscall_64` 的兜底分支） |
| 教材里的"逐参数调用"写法 | ⭐ v6.6：表项**只传 `pt_regs` 一个参数** |
| 不区分 ABI | ⭐ 先试 `do_syscall_x64`，失败再试 `do_syscall_x32` |

---

### 10. HFT 视角

| 机制 | HFT 含义 |
|------|---------|
| ⭐ **`array_index_nospec`** | 每次 syscall 都付一次 Spectre v1 缓解；这是"syscall 贵"的又一项固定成本 |
| ⭐ **7 个 `SYSCALL_WORK` 位测试** | 干净进程上是零成本；**容器（seccomp）里每次 syscall 都跑 BPF** —— 这是容器化 HFT 的隐性延迟 |
| ⭐ **`strace`** | 每次 syscall 两次进程停止 + 两次上下文切换。**生产环境绝不要挂 strace**，用 `perf` / `bpftrace` |
| ⭐ **内核栈 16KB** | 写内核模块 / eBPF 外的内核代码时，局部变量必须小；HFT 相关的内核旁路（如自定义字符设备）尤其注意 |
| **栈随机偏移 10 bit** | 每次 syscall 栈起点浮动 ≤1023 字节 → **栈访问的 cache 行不固定**，是 syscall 路径微小抖动的来源之一 |
| **`noinstr`** | syscall 入口段无法插 kprobe/ftrace → 想在这里埋探针只能用硬件断点或静态 tracepoint |

**实操三条：**

1. **热路径不要挂任何 syscall 追踪**。用 `perf record -e raw_syscalls:sys_enter` 采样，或用 BPF 在内核里聚合（成本在内核侧，不打断被观测进程）。
2. **容器里跑 HFT 要评估 seccomp 开销**：`SECCOMP_RET_ALLOW` 的 BPF 程序仍有固定成本。可用 `seccomp` 的 **bitmap 快速路径**（纯号码白名单时内核会跳过 BPF 执行）。
3. **别在内核栈上开大数组**。16KB 栈，一个 `char buf[4096]` 就用掉四分之一，再叠几层函数调用就可能 `stack overflow`。

---

### 触发方式（x86 演进）

| 机制 | 说明 |
|------|------|
| **`int $0x80`** | 经典 **软件中断**（32 位时代常见） |
| **`sysenter` / `syscall`** | 更快路径（现代 64 位主流为 **`syscall`**） |
| **`svc #0`** | **AArch64**（树莓派等 ARM64）的陷入指令，Supervisor Call — 与 `syscall` 同角色：主动陷入内核 |

> v6.6 里 `int $0x80` 的处理函数是 `do_int80_syscall_32()`（`arch/x86/entry/common.c:119`），仍完整保留——**32 位兼容层从未被移除**。

#### 寄存器约定

**x86（书中 32 位约定）**

| 寄存器 | 用途 |
|--------|------|
| **`eax`** | **系统调用号** 入 · **返回值** 出 |
| **`ebx, ecx, edx, esi, edi, ebp`** | 参数（按序） |

> **x86-64**：号在 **`rax`**，参数常用 **`rdi, rsi, rdx, r10, r8, r9`** — 思想相同：**寄存器传号与参**。
> ⚠️ 第 4 参数是 **`r10` 不是 `rcx`**（`rcx` 被 `syscall` 指令占用存返回地址），详见 [5.2 §3](./section-5.2-系统调用基础.md)。

---

→ 下一节 [§5.4 参数验证](./section-5.4-实现与参数验证.md) · [5.5 系统调用上下文](./section-5.5-系统调用上下文.md) · [5.6 添加与替代方案](./section-5.6-添加系统调用与替代方案.md)

> ↔ [ULK Ch10 §3 分派表与服务例程](../../../16-linux-kernel-deep/chapter-10-system-calls/notes/section-3-分派表与服务例程.md)

---

### 常见陷阱

1. 以为 `sys_call_table[nr]` 直接传 6 个参数——**v6.6 只传 `struct pt_regs *` 一个**，参数由 `SYSCALL_DEFINE` 生成的 `__se_sys_*` 层提取
2. 以为 `do_syscall_64` 只查一张表——它**先试 64 位表，失败再试 x32 表**，两张独立的表
3. 以为"号码非法就返回 -ENOSYS"这么简单——**注释明确说 "Invalid system call, but still a system call"**，audit/seccomp/tracepoint 已经记录过它了
4. 以为 seccomp 在 ptrace 之前跑——⭐ **seccomp 必须在 ptrace 之后**，否则 tracer 可绕过过滤（源码注释明文）
5. 以为内核栈很大——**x86_64 默认只有 16KB**（开 KASAN 才 32KB），大局部数组和深递归会爆栈
6. 以为 `strace` 只是"慢一点"——它让**每次 syscall 两次进程停止 + 两次上下文切换**，热路径上会慢到完全不可测
7. 以为内核栈随机偏移在 syscall 进入时抽——⭐ **在退出时抽**，目的是增大计时不确定性、缩短偏移在内存中的暴露时间
8. 以为 `do_syscall_64` 全程可插桩——它标了 **`noinstr`**，只有 `instrumentation_begin/end()` 之间的部分可以插探针

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** syscall 指令和 int 0x80 的区别？为什么现代 x86 用 syscall？

<details><summary>答案</summary>

int 0x80 是软件中断，需要查 IDT → 中断门 → 权限检查，开销约 200-400ns。syscall 是专门为快速系统调用设计的指令：不查 IDT、直接跳到 MSRs 指定的入口（LSTAR），开销约 50-100ns。现代内核默认用 syscall，int 0x80 仅保留兼容。

<details><summary>按 v6.6 修订/补充</summary>

机制层面的描述是对的，但**具体 ns 数字取决于硬件与缓解措施的开关状态**，不同机器上差异可达数倍，不宜当作定值引用。

**v6.6 实证补充：**

1. `int $0x80` 的处理函数 `do_int80_syscall_32()` 仍完整存在于 `arch/x86/entry/common.c:119`，**32 位兼容层从未被移除**：

```c
__visible noinstr void do_int80_syscall_32(struct pt_regs *regs)
{
	int nr = syscall_32_enter(regs);
	add_random_kstack_offset();
	nr = syscall_enter_from_user_mode(regs, nr);
	instrumentation_begin();
	do_syscall_32_irqs_on(regs, nr);
	instrumentation_end();
	syscall_exit_to_user_mode(regs);
}
```

2. `syscall` 指令的"快"体现在：CPU 硬件直接把返回地址存进 `rcx`、RFLAGS 存进 `r11`，并跳到 `MSR_LSTAR` 指定的地址，**完全绕过 IDT 查表和中断门权限检查**。

3. ⭐ **但两者的"后半程"完全一样**：都要 `add_random_kstack_offset()` → `syscall_enter_from_user_mode()` → 分派 → `syscall_exit_to_user_mode()`。所以差异**只在陷入的那几十条指令**，不在后续流程。

4. ⭐ 现代系统上，陷入指令本身的差异已经被**缓解措施的固定成本淹没**：PTI 的 2 次 CR3 切换 + IBRS + UNTRAIN_RET 的开销远大于"查不查 IDT"。这也是为什么[5.2 §4](./section-5.2-系统调用基础.md) 强调"syscall 贵主要贵在缓解措施"。

</details>
</details>

**Q2.** 系统调用处理程序为什么要检查 user_mode？

<details><summary>答案</summary>

内核需要验证请求来自用户态（而非内核态直接调用），防止内核代码绕过安全检查。`access_ok()` 验证用户态指针不会访问内核地址。如果内核代码能直接调 sys_read 传入内核指针，就绕过了所有安全检查。这是 Linux 安全模型的基础。

<details><summary>按 v6.6 修订/补充</summary>

补充一点：**v6.6 的入口函数命名本身就编码了这个约束**——它叫 `syscall_enter_**from_user_mode**()`。

```c
noinstr long syscall_enter_from_user_mode(struct pt_regs *regs, long syscall)
{
	long ret;
	__enter_from_user_mode(regs);
	instrumentation_begin();
	local_irq_enable();
	ret = __syscall_enter_from_user_work(regs, syscall);
	instrumentation_end();
	return ret;
}
```

`__enter_from_user_mode(regs)` 负责：
- 标记退出 `noinstr` / 上下文跟踪（RCU、lockdep、tracing）的状态切换
- 确保后续 `local_irq_enable()` 是安全的

**"内核态直接调 sys_read"会怎样？** 这正是内核里的经典 bug 模式：

| 问题 | 后果 |
|------|------|
| `__user` 指针标注失效 | 传入内核指针时 `access_ok()` 本该返回 false，但直接调用绕过了入口检查 |
| 内核页表可写 | 用户指针检查形同虚设，可读写任意内核内存 |
| `copy_from_user` 的缺页处理 | 内核地址缺页会 Oops 而不是返回 `-EFAULT` |

因此内核规范：**内核代码要调用 syscall 实现时，必须走内核内部函数（如 `kernel_read()`），不能调 `sys_read()`**。静态检查器（sparse）会对 `__user` 标注做检查。

</details>
</details>

**Q3.** `do_syscall_64` 里那句 `nr != -1` 是什么意思？`-1` 从哪来？

<details><summary>答案</summary>

```c
	if (!do_syscall_x64(regs, nr) && !do_syscall_x32(regs, nr) && nr != -1) {
		/* Invalid system call, but still a system call. */
		regs->ax = __x64_sys_ni_syscall(regs);
	}
```

`nr == -1` 表示"**这不是一个需要执行的 syscall，别调内核函数**"。它由 `syscall_enter_from_user_mode()` 返回，有三种来源：

| 来源 | 场景 |
|------|------|
| `syscall_user_dispatch(regs)` 返回真 | Syscall User Dispatch（Wine/Proton 模拟 Windows 二进制） |
| `ptrace_report_syscall_entry()` 返回非 0，或开了 `SYSCALL_WORK_SYSCALL_EMU` | tracer 决定不执行 |
| `__secure_computing()` 返回 `-1L` | seccomp BPF 判定拒绝 |

此时**内核函数一个都不执行**，但 `regs->ax` 已被 seccomp/ptrace 设置为指定的返回值（如 `SECCOMP_RET_ERRNO` 指定的 errno）。

⭐ **注释 "Invalid system call, but still a system call" 的含义**：即使号码非法（`do_syscall_x64` 和 `do_syscall_x32` 都返回 false），仍然要算作"发生了一次 syscall"——因为前面的 tracepoint / audit / seccomp 已经记录过它了。所以执行 `__x64_sys_ni_syscall(regs)`（返回 `-ENOSYS`）而不是静默跳过。

</details>

**Q4.** `array_index_nospec` 是干什么的？为什么每次 syscall 都要调它？

<details><summary>答案</summary>

```c
	if (likely(unr < NR_syscalls)) {
		unr = array_index_nospec(unr, NR_syscalls);
		regs->ax = sys_call_table[unr](regs);
		return true;
	}
```

**作用**：防 **Spectre v1**（边界检查绕过 / bounds-check bypass）。

**攻击原理**：CPU 推测执行时，即使 `unr < NR_syscalls` 这个分支判断还没算出结果，也会**推测性地**用恶意的 `unr` 去读 `sys_call_table[unr]`，把表外数据加载进 cache。随后分支判断失败、推测回滚，但**cache 里的痕迹留下来了**，攻击者可用侧信道（Flush+Reload 等）逐字节读出内核内存。

**防护原理**：`array_index_nospec(idx, sz)` 把索引**钳制**到 `[0, sz)` 范围内（用掩码或条件传送实现，不引入分支）。这样即使推测执行，读到的也永远是表内的合法地址，攻击者无法越界探测。

**为什么每次 syscall 都要**：因为 `sys_call_table` 是**按用户可控的号码索引**的数组——这正是 Spectre v1 的经典攻击面。每一次 syscall 都要付这个（很小的）成本。

> 这也是 [5.2 §4](./section-5.2-系统调用基础.md) 拆解的 syscall 开销的一部分：**缓解措施叠加**。

</details>

**Q5.** 为什么 seccomp 必须在 ptrace 之后执行？

<details><summary>答案</summary>

v6.6 `kernel/entry/common.c:71` 的源码注释直接给出了答案：

```c
	/* Do seccomp after ptrace, to catch any tracer changes. */
	if (work & SYSCALL_WORK_SECCOMP) {
		ret = __secure_computing(NULL);
		if (ret == -1L)
			return ret;
	}
```

**理由**：ptrace tracer 可以在 `ptrace_report_syscall_entry()` 里**修改被跟踪进程的寄存器**，包括 syscall 号码和参数。

如果顺序反过来（先 seccomp 后 ptrace）：

```
❌ 恶意顺序：
  1. 用户态发起 syscall A（无害，seccomp 白名单内）
  2. seccomp 检查 A → 通过
  3. ptrace tracer 把号码改成 B（危险，如 kexec_load）
  4. 执行 B  →  seccomp 形同虚设
```

```
✅ 正确顺序（v6.6）：
  1. 用户态发起 syscall A
  2. ptrace tracer 把号码改成 B
  3. seccomp 检查 B → 拒绝
  4. 不执行
```

所以 **"to catch any tracer changes"** = 让 seccomp 看到 tracer 改过之后的最终状态。

⭐ **紧接着的一行同样关键**：

```c
	/* Either of the above might have changed the syscall number */
	syscall = syscall_get_nr(current, regs);
```

ptrace 或 seccomp 都可能改写号码（seccomp 的 `SECCOMP_RET_ERRNO`/`SECCOMP_USER_NOTIF` 也会），所以必须**重新读一次**号码，然后才交给 tracepoint 和 audit 记录。

</details>

**Q6.** v6.6 有哪 7 个 `SYSCALL_WORK` 标志？它们存在哪？

<details><summary>答案</summary>

`include/linux/thread_info.h:42-57`：

```c
	SYSCALL_WORK_BIT_SECCOMP,
	SYSCALL_WORK_BIT_SYSCALL_TRACEPOINT,
	SYSCALL_WORK_BIT_SYSCALL_TRACE,
	SYSCALL_WORK_BIT_SYSCALL_EMU,
	SYSCALL_WORK_BIT_SYSCALL_AUDIT,
	SYSCALL_WORK_BIT_SYSCALL_USER_DISPATCH,
	SYSCALL_WORK_BIT_SYSCALL_EXIT_TRAP,
```

| 标志 | 触发者 | 作用 |
|------|--------|------|
| `SYSCALL_WORK_SECCOMP` | `seccomp(2)` / `prctl(PR_SET_SECCOMP)` | BPF 程序过滤 syscall |
| `SYSCALL_WORK_SYSCALL_TRACE` | `ptrace(PTRACE_SYSCALL)` | **strace 用的就是这个** |
| `SYSCALL_WORK_SYSCALL_EMU` | ptrace | 只通知 tracer，**不真执行** |
| `SYSCALL_WORK_SYSCALL_TRACEPOINT` | ftrace / perf / BPF | `raw_syscalls:sys_enter` |
| `SYSCALL_WORK_SYSCALL_AUDIT` | auditd 规则 | 审计日志 |
| `SYSCALL_WORK_SYSCALL_USER_DISPATCH` | `prctl(PR_SET_SYSCALL_USER_DISPATCH)` | Wine / Proton 模拟 Windows 二进制 |
| `SYSCALL_WORK_SYSCALL_EXIT_TRAP` | — | 退出时陷入通知 |

**存在哪**：`current_thread_info()->syscall_work` 位图（同文件 `:150-161` 提供 `set_bit` / `test_bit` / `clear_bit` 封装）。

⭐ **性能含义**：所有检查都是 `if (work & FLAG)` 一次位测试。一个既不 strace、又没 seccomp、又没 audit 的进程，`work == 0`，**六个分支全部不命中，零额外成本**。

反之：

| 进程类型 | 每次 syscall 额外成本 |
|---------|---------------------|
| 普通进程 | 零 |
| 开了 seccomp（容器 / 沙箱） | 跑一遍 BPF 程序 |
| 被 `strace` 跟踪 | ⭐ 两次进程停止 + 两次 tracer 上下文切换 |

</details>

**Q7.** 内核栈有多大？`add_random_kstack_offset` 的偏移为什么在 syscall **退出时**才抽取？

<details><summary>答案</summary>

**大小**（v6.6 `arch/x86/include/asm/page_64_types.h:15-16`）：

```c
#define THREAD_SIZE_ORDER	(2 + KASAN_STACK_ORDER)
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)
```

| 配置 | ORDER | 大小 |
|------|-------|------|
| 默认 | 2 | **16 KB** |
| 开 KASAN | 3 | **32 KB** |

⭐ 16KB 很小——这是内核编码规范**禁止大局部数组和深递归**的直接原因。

**偏移上限**（`include/linux/randomize_kstack.h`）：

```c
#define KSTACK_OFFSET_MAX(x)	((x) & 0x3FF)
```

**10 bit 熵** = 偏移最多 **1023 字节**。用 static key 控制，关闭时零开销。

⭐⭐ **为什么在退出时抽（而不是进入时）**——源码注释的两条理由：

> ```
>  * This should only be used during syscall exit when interrupts and
>  * preempt are disabled. This position in the syscall flow is done to
>  * frustrate attacks from userspace attempting to learn the next offset:
>  * - Maximize the timing uncertainty visible from userspace: if the
>  *   offset is chosen at syscall entry, userspace has much more control
>  *   over the timing between choosing offsets. "How long will we be in
>  *   kernel mode?" tends to be more difficult to predict than "how long
>  *   will we be in user mode?"
>  * - Reduce the lifetime of the new offset sitting in memory during
>  *   kernel mode execution. Exposure of "thread-local" memory content
>  *   (e.g. current, percpu, etc) tends to be easier than arbitrary
>  *   location memory exposure.
> ```

| 理由 | 说明 |
|------|------|
| ⭐ **增大计时不确定性** | 进入时抽 → 用户态能精确控制"抽偏移"到下次 syscall 的时间间隔；退出时抽 → 攻击者必须预测"这次进内核会待多久"，难得多 |
| ⭐ **缩短暴露窗口** | 退出时抽完立刻回用户态，新偏移在内核内存里停留时间最短；且 `current` / percpu 这类"线程局部"内存比任意内存更容易被泄露 |

**精妙之处**：这个安全性提升**不花任何运行时成本**——只是把抽随机数的位置挪了一下。

**副作用**：每次 syscall 的栈起点浮动 ≤1023 字节，栈访问的 cache 行不固定，是 syscall 路径微小抖动的来源之一。

</details>

**Q8.** `sys_call_table` 是怎么生成的？为什么表里写 `sys_read` 而实际符号是 `__x64_sys_read`？

<details><summary>答案</summary>

`arch/x86/entry/syscall_64.c` 全文只有 451 字节：

```c
#define __SYSCALL(nr, sym) extern long __x64_##sym(const struct pt_regs *);
#include <asm/syscalls_64.h>
#undef __SYSCALL

#define __SYSCALL(nr, sym) __x64_##sym,

asmlinkage const sys_call_ptr_t sys_call_table[] = {
#include <asm/syscalls_64.h>
};
```

**四处精妙：**

| 技巧 | 说明 |
|------|------|
| ⭐ **X-macro** | 同一头文件 **include 两次**，配不同的 `__SYSCALL` 定义：第一次生成 extern 声明，第二次生成数组元素 |
| ⭐ **`const`** | 表本身只读，防止运行时被篡改（rootkit 常见攻击点） |
| ⭐ **`__x64_` 前缀** | 与[5.2](./section-5.2-系统调用基础.md)呼应：表里写 `sys_read`，实际符号是 `__x64_sys_read` |
| **自动生成** | `asm/syscalls_64.h` 由 `scripts/syscalltbl.sh` 在构建时从 `syscall_64.tbl` 生成，**不在源码树里** |

**为什么实际符号带 `__x64_` 前缀**（对应 `CONFIG_ARCH_HAS_SYSCALL_WRAPPER`）：

```
sys_call_table[nr](regs)
    ▼
__x64_sys_read(regs)               ← 表里存的，签名统一为 long (*)(const struct pt_regs *)
    ▼
__se_sys_read(fd, buf, count)      ← 从 regs->di/si/dx 取参数，做符号/零扩展
    ▼
__do_sys_read(fd, buf, count)      ← 你写的函数体
```

| 层 | 职责 |
|----|------|
| `__x64_sys_*` | 统一签名，供表调用 |
| `__se_sys_*` | **s**ign **e**xtend：参数提取 + 符号扩展（安全性关键） |
| `__do_sys_*` | 真正实现 |

⭐ **`__se_sys_*` 为什么必要**：C 语言无法表达"这个 32 位参数在 64 位寄存器里"。例如 `int fd` 从 `regs->di` 取低 32 位时必须正确符号扩展，否则 `-1` 会变成无符号的 `0xFFFFFFFF`。这层由宏自动生成，避免手写出错。

</details>

**Q9.** 教材常写的"逐参数调用"写法，与 v6.6 实际有哪些差别？

<details><summary>答案</summary>

| 教材版（残缺） | v6.6 实际 |
|---------------|----------|
| `if (nr >= NR_syscalls) return -ENOSYS;` | `if (likely(unr < NR_syscalls)) {...} return false;`（**正数**判断 + `likely`） |
| 无 | ⭐ `array_index_nospec(unr, NR_syscalls)`（**Spectre v1 防护**） |
| `return -ENOSYS` | `regs->ax = __x64_sys_ni_syscall(regs)`（由 `do_syscall_64` 兜底分支执行） |
| 逐参数调用（6 个参数） | ⭐ 表项**只传 `pt_regs` 一个参数** |
| 不区分 ABI | ⭐ 先试 `do_syscall_x64`，失败再试 `do_syscall_x32` |
| 无 | ⭐ 负数号码处理：`unsigned int unr = nr` 把负数变成巨大正数，省一次显式判断 |
| 无 | ⭐ `noinstr` + `instrumentation_begin/end()`（禁止整段插桩） |
| 无 | ⭐ `add_random_kstack_offset()` |

v6.6 完整实现（`arch/x86/entry/common.c:73-87`）：

```c
__visible noinstr void do_syscall_64(struct pt_regs *regs, int nr)
{
	add_random_kstack_offset();
	nr = syscall_enter_from_user_mode(regs, nr);

	instrumentation_begin();

	if (!do_syscall_x64(regs, nr) && !do_syscall_x32(regs, nr) && nr != -1) {
		/* Invalid system call, but still a system call. */
		regs->ax = __x64_sys_ni_syscall(regs);
	}

	instrumentation_end();
	syscall_exit_to_user_mode(regs);
}
```

</details>

**Q10.** 为什么 `strace` 会让程序慢到不可测？生产环境该用什么？

<details><summary>答案</summary>

**机制**：`strace` 用 `ptrace(PTRACE_SYSCALL)`，这会置位 `SYSCALL_WORK_SYSCALL_TRACE`。于是**每一次 syscall**（进入和退出各一次）：

1. 被跟踪进程在 `syscall_enter_from_user_mode()` 里停下
2. 唤醒 tracer（strace 进程）——**一次上下文切换**
3. strace 用 `PTRACE_GETREGS` 读寄存器——**一次系统调用**
4. strace 恢复被跟踪进程——**又一次上下文切换**
5. syscall 本身执行
6. **退出时再来一遍**（步骤 1-4）

即：**每次 syscall = 至少 4 次上下文切换 + 若干次 ptrace 调用**。

对比干净路径：只是 `if (work & SYSCALL_WORK_SYSCALL_TRACE)` 一次位测试，不命中。

| 进程类型 | 每次 syscall 额外成本 |
|---------|---------------------|
| 干净进程 | ⭐ **零**（一次位测试） |
| 被 `strace` 跟踪 | ⭐ 4+ 次上下文切换 |

一个热路径上每秒百万次 syscall 的程序（HFT 行情处理、数据库），挂上 `strace` **会慢到完全无法反映真实行为**，且测出的延迟分布完全失真。

**生产环境替代方案：**

| 工具 | 机制 | 开销 |
|------|------|------|
| ⭐ **`perf record -e raw_syscalls:sys_enter`** | tracepoint + 采样/聚合在内核侧 | 只在采样点付费 |
| ⭐ **`bpftrace` / BCC** | BPF 程序在内核里聚合，只把结果传回用户态 | 每 syscall 微秒级以下 |
| `strace -c` | 只统计不逐条打印 | 仍是 ptrace，只是减少了输出开销 |
| `ltrace` | 库函数级 | 同 ptrace 问题 |

> ⭐ **关键区别**：`perf` / BPF 是**内核侧聚合**，不把控制权交回用户态 tracer；`strace` 是**用户态 tracer**，每次都要切换过去。

</details>

</details>

---

> ↔ [ULK Ch10 §3 分派表与服务例程](../../../16-linux-kernel-deep/chapter-10-system-calls/notes/section-3-分派表与服务例程.md)
---
