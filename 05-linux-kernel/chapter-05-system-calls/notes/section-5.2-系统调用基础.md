## ② 系统调用基础 · Numbers & Naming

> 承接 [5.1 与内核通信](./section-5.1-与内核通信.md)。
> 本节回答：**系统调用号怎么分配、为什么永不回收、以及一次 syscall 的开销到底花在哪。**

---

### 1. 系统调用号：v6.6 x86_64 实证

每一个内核提供的服务，分配一个独一无二的 **数字编号（系统调用号）**。内核内部维护一张大表：**`sys_call_table`**。

v6.6 `arch/x86/entry/syscalls/syscall_64.tbl` 前 15 行原文：

```
0	common	read			sys_read
1	common	write			sys_write
2	common	open			sys_open
3	common	close			sys_close
4	common	stat			sys_newstat
5	common	fstat			sys_newfstat
6	common	lstat			sys_newlstat
7	common	poll			sys_poll
8	common	lseek			sys_lseek
9	common	mmap			sys_mmap
10	common	mprotect		sys_mprotect
11	common	munmap			sys_munmap
12	common	brk			sys_brk
13	64	rt_sigaction		sys_rt_sigaction
14	common	rt_sigprocmask		sys_rt_sigprocmask
```

| 表字段 | 含义 |
|--------|------|
| 第 1 列 | 号码 |
| 第 2 列 | ABI：`common` / `64`（仅 64 位）/ `x32`（仅 x32） |
| 第 3 列 | 用户可见名字 |
| 第 4 列 | 内核入口符号 |

| | |
|--|--|
| 数组下标 | = 系统调用号 |
| 数组内容 | = 对应内核处理函数指针 |

#### v6.6 x86_64 的精确数字

| 指标 | 数值 |
|------|------|
| 表中总条目 | **401** |
| 其中 64 位可见 | **365**（`common` 317 + `64` 专属 48） |
| x32 专属 | 36 |
| 最大号码 | **547** |
| `__NR_syscalls` | **453** |
| 表内 `sys_ni_syscall` 存根 | **1 个**（156 号 `_sysctl`） |

> ⚠️ **统计 syscall 数量要看条目数，不是最大号码** —— 号码会跳（见下）。

#### 表头注释里的两条硬规则

```
# don't use numbers 387 through 423, add new calls after the last
# 'common' entry
#
# Due to a historical design error, certain syscalls are numbered differently
# in x32 as compared to native x86_64.  These syscalls have numbers 512-547.
# Do not add new syscalls to this range.  Numbers 548 and above are available
# for non-x32 use.
```

#### ⭐ 号码表上的三类"洞"

| 类型 | 位置 | 成因 |
|------|------|------|
| **x32 跳跃洞** | **335–423（89 个号码）** | 为避开 x32 的 512-547 段而预留，详见 [Ch 1.1](../../chapter-01-intro/notes/section-1.1-Unix-的历史.md) |
| **架构差异洞** | 各架构不同 | 同一号码在不同架构上是不同 syscall（ABI 不统一） |
| **历史移除洞** | 如 156 号 | 填 `sys_ni_syscall`，**号码不回收** |

> ⭐ **x32 洞的真正原因**（v5.1 的表头注释原文，v6.6 已删除）：
> `# x32-specific system call numbers start at 512 to avoid cache impact for native 64-bit operation.`
> —— syscall 表是**按号索引的数组**，x32 号码交错会拉大数组跨度，恶化原生 64 位路径的 cache 局部性。

#### ⭐ 表头另一条容易被忽略的信息

```
# The __x64_sys_*() stubs are created on-the-fly for sys_*() system calls
```

表里写的是 `sys_read`，但**实际链接进 `sys_call_table` 的符号是 `__x64_sys_read`**。中间的包装层由 `SYSCALL_DEFINE` 宏自动生成，负责 64 位参数的符号/零扩展。

---

### 2. `-ENOSYS` 的两条不同路径

| 规则 | 说明 |
|------|------|
| 每个 syscall **唯一编号** | 如 x86_64 `__NR_read = 0` |
| **号一旦分配永不回收** | 保证 **ABI 稳定** |
| 历史 syscall 被移除 | 槽位填 **`sys_ni_syscall()`** — 只返回 **`-ENOSYS`** |

但"返回 `-ENOSYS`"其实有 **两条完全不同的实现路径**，这是多数教材不讲的部分。

#### 路径 A：号码被永久废弃 → 表里直填

v6.6 `kernel/sys_ni.c:22-25`——整个函数就这么点：

```c
/*
 * Non-implemented system calls get redirected here.
 */
asmlinkage long sys_ni_syscall(void)
{
	return -ENOSYS;
}
```

在 `syscall_64.tbl` 里，全表**只有一处**直接引用它：

```
156	64	_sysctl			sys_ni_syscall
```

#### 路径 B：功能未编译进来 → 弱符号在链接期填

```c
/* kernel/sys_ni.c:26 */
#define COND_SYSCALL(name) cond_syscall(sys_##name)
```

`sys_ni.c` 里有 **217 条** `COND_SYSCALL(...)`，例如：

```c
COND_SYSCALL(io_uring_setup);
COND_SYSCALL(io_uring_enter);
COND_SYSCALL(io_uring_register);
COND_SYSCALL(futex);
COND_SYSCALL(clone3);
COND_SYSCALL(epoll_pwait2);
COND_SYSCALL(quotactl_fd);
...
```

**机制**：`cond_syscall()` 展开成一个 **弱符号别名**指向 `sys_ni_syscall`。如果真正的实现因为 `CONFIG_*` 未开启而没被编译进 vmlinux，链接器就用这个弱符号兜底。

| | 路径 A（号码废弃） | 路径 B（功能未编译） |
|---|------------------|-------------------|
| 触发 | syscall 从内核彻底消失 | syscall 还在，但依赖的 CONFIG 关闭 |
| 位置 | `syscall_64.tbl` 表项 | `kernel/sys_ni.c` 的 `COND_SYSCALL` |
| 发生时机 | 编译表时 | **链接期**（弱符号解析） |
| v6.6 数量（x86_64） | **1 个**（156 `_sysctl`） | **217 个** |
| 用户态可见 | 都是 `-ENOSYS` | 都是 `-ENOSYS` |

> ⭐ **HFT/嵌入式含义**：精简内核（关掉 io_uring、fanotify、kexec 等）不会让这些号码报错崩溃，而是优雅返回 `-ENOSYS`——**这层弱符号兜底正是"内核可裁剪"的基础设施**。glibc 会据此退化到老实现（如 `openat` 不可用时退 `open`）。

---

### 3. 陷入指令：x86_64 上三条路径并存

| 架构 | 指令 | 备注 |
|------|------|------|
| x86_32 | `int 0x80`（软中断） | 最慢，走完整 IDT 查表 |
| x86_32（新） | `sysenter` / `sysexit` | Intel 的快速路径 |
| **x86_64** | **`syscall` / `sysret`** | AMD 提出的专用快速路径，跳过 IDT |
| ARM64 | `svc #0` | |

**调用流程（极简版）：**

1. 用户程序把 **系统调用号、调用参数** 放入 CPU 指定寄存器；
2. 触发 CPU 特权切换指令，陷入内核；
3. 内核取出调用号，查 `sys_call_table`，执行对应内核函数；
4. 执行完成，把返回值放入寄存器，切回用户态继续运行。

#### x86_64 的寄存器约定

| 用途 | 寄存器 |
|------|--------|
| 系统调用号 | `rax` |
| 参数 1~6 | `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` |
| 返回值 | `rax` |
| **`syscall` 指令破坏的** | `rcx`（存返回 RIP）、`r11`（存 RFLAGS） |

> ⚠️ 注意参数 4 是 **`r10` 而不是 `rcx`** —— 因为 `syscall` 指令本身要用 `rcx` 保存返回地址。这是 x86_64 syscall 与 System V 函数调用 ABI 的**唯一差异**，写内联汇编时最容易踩。

#### ⭐ 一个安全细节：号码按 int 处理

v6.6 `arch/x86/entry/entry_64.S:115-116`：

```asm
	/* Sign extend the lower 32bit as syscall numbers are treated as int */
	movslq	%eax, %rsi
```

只取 `eax` 的低 32 位并**符号扩展**成 64 位。因此负数号码会变成一个巨大的正数索引，在 `do_syscall_64` 的边界检查（`nr < NR_syscalls`）处被拒绝，**不可能索引到表外**。

---

### 4. ⭐ syscall 到底贵在哪：逐步拆解 `entry_SYSCALL_64`

"syscall 比函数调用慢两个数量级"人人都会说，但**开销具体花在哪**要读汇编。以下是 v6.6 `arch/x86/entry/entry_64.S:87-227` 的**真实指令序列**。

#### 进入路径（:87-120）

```asm
SYM_CODE_START(entry_SYSCALL_64)
	UNWIND_HINT_ENTRY
	ENDBR                                             /* ① CET：ENDBR64 着陆点 */

	swapgs                                            /* ② 换 GS 基址（用户GS → 内核GS） */
	movq	%rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)
	SWITCH_TO_KERNEL_CR3 scratch_reg=%rsp             /* ③ 换页表 CR3（PTI） */
	movq	PER_CPU_VAR(pcpu_hot + X86_top_of_stack), %rsp  /* ④ 切内核栈 */

	/* Construct struct pt_regs on stack */
	pushq	$__USER_DS                                /* ⑤ 手工构造 pt_regs（6 次 push） */
	pushq	PER_CPU_VAR(cpu_tss_rw + TSS_sp2)
	pushq	%r11
	pushq	$__USER_CS
	pushq	%rcx
	pushq	%rax

	PUSH_AND_CLEAR_REGS rax=$-ENOSYS                  /* ⑥ 全寄存器压栈 + 清零 */

	movq	%rsp, %rdi
	movslq	%eax, %rsi                                /* ⑦ 号码符号扩展为 int */

	IBRS_ENTER                                        /* ⑧ Spectre v2 防护 */
	UNTRAIN_RET                                       /* ⑨ Retbleed 防护 */

	call	do_syscall_64                             /* ⑩ 真正干活 */
```

#### 返回路径（:198-227）

```asm
syscall_return_via_sysret:
	IBRS_EXIT                                         /* ① */
	POP_REGS pop_rdi=0                                /* ② 全寄存器出栈 */

	movq	%rsp, %rdi
	movq	PER_CPU_VAR(cpu_tss_rw + TSS_sp0), %rsp    /* ③ 切 trampoline 栈 */
	pushq	RSP-RDI(%rdi)
	pushq	(%rdi)

	STACKLEAK_ERASE_NOCLOBBER                         /* ④ 清空内核栈（防信息泄露） */
	SWITCH_TO_USER_CR3_STACK scratch_reg=%rdi         /* ⑤ 换回用户页表 CR3 */

	popq	%rdi
	popq	%rsp
	swapgs                                            /* ⑥ 换回用户 GS */
	sysretq                                           /* ⑦ 返回用户态 */
```

#### ⭐ 开销清单

| 步骤 | 成本性质 |
|------|---------|
| **2 次 CR3 写入**（`SWITCH_TO_KERNEL_CR3` / `SWITCH_TO_USER_CR3_STACK`） | ⭐ **最贵**——写 CR3 可能触发 TLB 失效；这是 Meltdown 的 PTI 代价 |
| **2 次 `swapgs`** | 中段；切换 GS 段基址以访问 percpu 数据 |
| **`IBRS_ENTER` / `IBRS_EXIT`** | Spectre v2 防护，间接分支预测限制 |
| **`UNTRAIN_RET`** | Retbleed 防护，返回地址的"去训练"序列 |
| **`PUSH_AND_CLEAR_REGS` / `POP_REGS`** | 全套通用寄存器压栈/出栈 + 清零（防 Spectre 的寄存器残留泄露） |
| **`STACKLEAK_ERASE_NOCLOBBER`** | 清空用过的内核栈（防栈内容泄露） |
| 手工构造 `pt_regs`（6 次 `pushq`） | 中段 |
| 2 次栈切换（内核栈 / trampoline 栈） | 小段 |

> ⭐⭐ **关键认知**：syscall 的开销**主要不是"特权级切换"本身**，而是 2018 年 Spectre/Meltdown 之后叠加的**一整套侧信道缓解措施**——PTI（双页表）、IBRS、UNTRAIN_RET、寄存器清零、栈清空。
>
> 这也是为什么**"少做 syscall"比"让 syscall 更快"更有效**：io_uring 的收益来自**批量摊薄**（一次 `io_uring_enter` 提交/收割多个 IO），而不是让单次陷入变快。详见 [5.6 添加系统调用与替代方案](./section-5.6-添加系统调用与替代方案.md)。

---

### 5. 返回值约定与 ⭐ `MAX_ERRNO = 4095`

| 内核约定（惯例） | 含义 |
|------------------|------|
| 正数 / 0 | 成功 |
| 负数 | 错误码（内核负错误码） |

libc 会把负错误码转换成全局变量 **`errno`**（用户态看到的是正 errno + 函数返回 -1）。

#### ⭐ 返回"指针"的 syscall 怎么办？

有些 syscall 成功时返回一个**地址**（如 `mmap`）。它们没法用"负数"表示错误——`0xFFFFFFFFFFFFF000` 以上的地址虽然通常是内核空间，但用户也可能合法映射。

内核的解法：v6.6 `include/linux/err.h:18-49`：

```c
#define MAX_ERRNO	4095

/*
 * IS_ERR_VALUE - Detect an error pointer.
 * ...
 */
#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline long __must_check PTR_ERR(__force const void *ptr)
{
	return (long) ptr;
}
```

| 宏 | 定义 | 作用 |
|----|------|------|
| `MAX_ERRNO` | `4095` | 最大错误码 |
| `IS_ERR_VALUE(x)` | `(unsigned long)(x) >= (unsigned long)-4095` | 判断是否为错误指针 |
| `IS_ERR(p)` | `IS_ERR_VALUE((unsigned long)ptr)` | 常用包装 |
| `PTR_ERR(p)` | `(long)ptr` | 从错误指针取出负错误码 |
| `ERR_PTR(e)` | `(void *)(long)e` | 把负错误码编码成指针 |

**内存布局直觉：**

```
0xFFFFFFFFFFFFFFFF  ┌─────────────────────┐
                    │  错误指针区间        │  0xFFFFFFFFFFFFF001 ~ 0xFFFFFFFFFFFFFFFF
0xFFFFFFFFFFFFF001  │  -1 ~ -4095         │  ← 最后一页，永不映射合法内存
                    ├─────────────────────┤
                    │  合法地址空间        │  ← 用户可映射的最大地址 < 0xFFFFFFFFFFFFF000
0x0000000000000000  └─────────────────────┘
```

> ⭐ **为什么是 4095？** 因为地址空间的**最后一页**永不映射（`TASK_SIZE` 以下才是用户可映射范围）。把 `-1 ~ -4095` 编码成"最后一页内的假指针"，就与所有合法指针天然不冲突。判断只需一次无符号比较，零成本。

| syscall 类别 | 成功返回 | 失败返回 | 用户态判定 |
|-------------|---------|---------|-----------|
| 返回 `long` 的（read/write/...） | ≥ 0 的值 | `-errno` | `if (ret < 0)` |
| 返回指针的（mmap/...） | 合法地址 | `ERR_PTR(-errno)` | `if (IS_ERR(p))` / glibc 转成 `-1` + `errno=ENOMEM` |

---

### 6. ⭐ `asmlinkage` 的真相：x86_64 上它是**空宏**

原文常说 *"`asmlinkage` = 参数仅从栈取"*。这**只对 i386 成立**，在 x86_64 上是错的。

#### x86 的定义（v6.6 `arch/x86/include/asm/linkage.h:19-21`）

```c
#ifdef CONFIG_X86_32
#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))
#endif /* CONFIG_X86_32 */
```

注意 `#ifdef CONFIG_X86_32` —— **64 位下这个宏根本不存在**。

#### 于是落到通用定义（v6.6 `include/linux/linkage.h:14-22`）

```c
#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif
```

| 架构/配置 | `asmlinkage` 展开成 | 实际效果 |
|----------|-------------------|---------|
| **x86_64** | `CPP_ASMLINKAGE` → **空** | ⭐ **毫无作用**，纯历史遗迹 |
| **x86_32** | `extern "C" __attribute__((regparm(0)))` | 强制参数全部走栈 |
| C++ 编译 | `extern "C"` | 关闭 name mangling |
| 多数其他架构 | 空 | 同 x86_64 |

> ⭐⭐ **`regparm(0)` 是 i386 时代的产物**：i386 通用寄存器太少（8 个），`fastcall`/`regparm` 之争最终让内核选择"参数全走栈"以简化汇编入口。x86_64 有 16 个寄存器、ABI 本来就用寄存器传参（`rdi/rsi/rdx/...`），**不需要也不存在这个属性**。
>
> 所以：**`asmlinkage` 在现代 x86_64 内核里只是一个语义标注**（告诉读者"这是从汇编入口调用的"），编译层面什么都不做。

| 约定 | 说明 |
|------|------|
| **`asmlinkage`** | x86_64 上为空宏；i386 上强制栈传参（`regparm(0)`） |
| **`sys_` 前缀** | 用户 `bar()` → 内核 `sys_bar()`；v6.6 实际入口符号是 **`__x64_sys_bar`**（由 `SYSCALL_DEFINE` 宏生成包装层） |

```c
/* 概念示意 */
asmlinkage long sys_read(unsigned int fd, char __user *buf, size_t count);
/* v6.6 实际：SYSCALL_DEFINE3(read, ...) 展开出 __x64_sys_read()，
   内部再调用 __se_sys_read() → __do_sys_read() */
```

---

### 7. ⭐ vDSO：不陷入内核的"系统调用"

如果 syscall 这么贵，那像 `clock_gettime()` 这种**只读一个内核变量**的调用，也要付出全套 PTI + IBRS 的代价吗？

内核的答案：**vDSO（virtual Dynamic Shared Object）** —— 把一小段内核代码 + 一个数据页**映射到每个进程的用户地址空间**，让部分"系统调用"完全在用户态执行。

#### v6.6 x86_64 vDSO 导出的全部符号

`arch/x86/entry/vdso/vdso.lds.S` 的 VERSION 段：

```
VERSION {
	LINUX_2.6 {
	global:
		clock_gettime;
		__vdso_clock_gettime;
		gettimeofday;
		__vdso_gettimeofday;
		getcpu;
		__vdso_getcpu;
		time;
		__vdso_time;
		clock_getres;
		__vdso_clock_getres;
#ifdef CONFIG_X86_SGX
		__vdso_sgx_enter_enclave;
#endif
	local: *;
	};
}
```

| 导出符号 | 作用 | 是否真正避免陷入 |
|---------|------|----------------|
| **`clock_gettime`** | 高精度时间 | ⭐ **是**（`CLOCK_MONOTONIC` 等纯计算时钟） |
| **`gettimeofday`** | 墙上时间 | ⭐ **是** |
| **`clock_getres`** | 时钟分辨率 | ⭐ **是**（返回常量） |
| **`time`** | 秒级时间 | ⭐ **是** |
| **`getcpu`** | 当前 CPU / NUMA 节点 | ⭐ **是**（读 percpu 页） |
| `__vdso_sgx_enter_enclave` | SGX enclave 进入 | 条件编译（`CONFIG_X86_SGX`） |

#### ⭐ 原理：内核数据页 + 用户态计算

```c
/* lib/vdso/gettimeofday.c:266-269 */
static __maybe_unused int
__cvdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts)
{
	return __cvdso_clock_gettime_data(__arch_get_vdso_data(), clock, ts);
}
```

```
      用户态                              内核态
  ┌──────────────────┐              ┌──────────────────┐
  │ glibc            │              │ 时间keeping      │
  │ clock_gettime()  │              │ 更新 vdso_data   │
  │   ↓              │              │  （每 tick）     │
  │ __vdso_clock_    │   只读映射    │                  │
  │   gettime()      │ ◄────────────┤ vdso_data 页     │
  │   ↓ 纯算术       │              │ (seqcount 保护)  │
  │ 读 vdso_data +   │              └──────────────────┘
  │ 用上次的多项式   │
  │ 插值算纳秒       │
  └──────────────────┘
        ↑ 全程无特权切换、无 CR3 切换
```

| 关键点 | 说明 |
|--------|------|
| 数据来源 | 内核把 `vdso_data` 页**只读**映射到每个进程 |
| 并发保护 | ⭐ **seqcount**：用户态读序列，读到奇数序号就重试（无锁、无原子） |
| 为何安全 | 用户态**只能读**，改不了内核时间 |
| glibc 透明 | 用户调 `clock_gettime()`，glibc 自动优先走 vDSO；**代码零改动** |

> ⭐ **注意不是所有时钟都走 vDSO**：`CLOCK_MONOTONIC`、`CLOCK_REALTIME`、`CLOCK_MONOTONIC_RAW`、`CLOCK_BOOTTIME` 等纯计算时钟可以；涉及复杂换算或高精度硬件读的会**回退到真 syscall**（vDSO 内部会 `syscall` 兜底）。

---

### 8. HFT 视角

| 机制 | HFT 含义 |
|------|---------|
| **syscall 开销** | 单次陷入含 2 次 CR3 切换 + IBRS + 寄存器清零 ≈ **数百到上千 cycles**，且**尾延迟抖动大**（TLB 失效、缓存冷） |
| ⭐ **vDSO `clock_gettime`** | HFT 打时间戳的**唯一正确选择**。走 vDSO 是 **~20-30 cycles**，走 syscall 是 **~1000+ cycles**，差 1~2 个数量级 |
| **`getcpu`** | 每笔订单打 CPU 号做核绑定校验时，用 vDSO 版本零成本 |
| **io_uring** | 收益是**批量摊薄**（一次陷入处理 N 个 IO），不是"更快的 syscall" |
| **errno / `MAX_ERRNO`** | 热路径上别用返回指针再判 `IS_ERR` 的封装，直接判返回值更快 |
| **`-ENOSYS` 兜底** | 精简内核里 io_uring 等可选，glibc/应用要能优雅退化 |

**实操三条：**

1. ⭐ **打时间戳用 `clock_gettime(CLOCK_MONOTONIC)`**，glibc 会走 vDSO；**别用 `gettimeofday()`**（同样走 vDSO 但精度/语义更弱），**更别用 `time()`**（只到秒）。
2. **批量化**：写日志/投递消息攒批再 `write()`；网络 IO 用 `io_uring` 或 `sendmmsg()` 摊薄陷入成本。
3. **验证**：`strace -c -T ./prog` 看 syscall 的时间占比；`perf stat -e raw_syscalls:sys_enter` 数陷入次数。

---

→ 用户态查号：`unistd.h` / `asm/unistd.h` · `strace` 可见实际号 · 下一节 [§5.3 入口处理](./section-5.3-系统调用处理程序.md) · [5.4 实现与参数验证](./section-5.4-实现与参数验证.md) · [5.6 添加与替代方案](./section-5.6-添加系统调用与替代方案.md)

> ↔ [ULK Ch10 §2 POSIX-API与系统调用](../../../16-linux-kernel-deep/chapter-10-system-calls/notes/section-2-POSIX-API与系统调用.md)

---

### 常见陷阱

1. 以为 syscall 号在所有架构相同——x86_64 的 `read=0`，但 ARM64/MIPS 完全不同，源码可移植、二进制不可移植
2. 以为 syscall 数量等于最大号码——**号码会跳**，v6.6 x86_64 最大号 547 但只有 365 个 64 位 syscall
3. ⭐ 以为 `asmlinkage` 在 x86_64 上是"参数走栈"——**它在 x86_64 上是空宏**，只有 i386 才展开成 `regparm(0)`
4. ⭐ 以为所有 syscall 都要陷入内核——**vDSO 的 5 个符号在用户态执行**，零特权切换
5. 以为 `-ENOSYS` 只有一个来源——有**两条路径**：表内直填 `sys_ni_syscall`（1 个）+ 弱符号 `cond_syscall`（217 个）
6. 以为 io_uring 是"更快的 syscall"——它是**更少的 syscall**，收益来自批量摊薄
7. 以为所有 `clock_gettime` 都走 vDSO——部分时钟会**回退到真 syscall**
8. 写内联汇编时把第 4 个参数放进 `rcx`——**x86_64 syscall 第 4 参数是 `r10`**，`rcx` 被 `syscall` 指令占用存返回地址

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 系统调用号存在哪里？为什么每个架构不同？

<details><summary>答案</summary>

syscall 号定义在 `arch/*/include/uapi/asm/unistd.h`。x86_64 的 read=0, write=1, open=2...。不同架构不同是因为历史原因（x86 用 int 0x80，ARM 用 SVC 指令）。用户态 libc 的 read() 内部根据架构填入正确的 syscall 号。这保证了源码可移植但二进制不可移植。

<details><summary>按 v6.6 修订/补充</summary>

补充实证数字（v6.6 `arch/x86/entry/syscalls/syscall_64.tbl`）：

| 指标 | 数值 |
|------|------|
| 表内条目总数 | **401** |
| 64 位可见 | **365**（`common` 317 + `64` 专属 48） |
| x32 专属 | 36 |
| 最大号码 | **547** |
| `__NR_syscalls` | 453 |

⚠️ **数量 ≠ 最大号码**：号码表有著名的 **335–423 空洞（89 个号码）**，成因见 v5.1 表头注释——为避开 x32 的 512-547 段。判断"有多少 syscall"必须数条目。

另外，表里第 4 列写的是 `sys_read`，但**实际符号是 `__x64_sys_read`**。表头注释原文：

> "The `__x64_sys_*()` stubs are created on-the-fly for `sys_*()` system calls"

这层包装由 `SYSCALL_DEFINE` 宏生成，负责 64 位参数的符号/零扩展。

</details>
</details>

**Q2.** 为什么现代内核不鼓励新增系统调用？

<details><summary>答案</summary>

新增 syscall 是永久 ABI 承诺：一旦合入 mainline 就不能删除/改语义（会破坏用户态程序）。替代方案：1) io_uring（一个 syscall 搞定任意 IO 操作）；2) eBPF（用户态程序注入内核运行）；3) /proc 或 /sys 接口（不需要新 syscall 号）。

<details><summary>按 v6.6 修订/补充</summary>

补充两点：

**① 增长数据佐证"克制"**：v4.9 的 64 位 syscall = 332 个，v6.6 = 365 个，**7 年净增 33 个 ≈ 每年 5 个**。这个增速远低于内核其他部分的扩张。

**② "不鼓励"的机制性代价**：从 §4 的 `entry_SYSCALL_64` 拆解可以看到，每新增一个 syscall，就要：
- 在 `sys_call_table` 占一个永久槽位
- 为每个架构的 `syscall.tbl` 各加一行（x86_64 / x32 / arm64 / riscv / ...），漏一个就是 ABI 分裂
- 承担永久的 `SYSCALL_DEFINE` 参数校验与 compat 层维护

**③ io_uring 的定位要说准**：io_uring **不是"更快的 syscall"，是"更少的 syscall"**。单次陷入的开销（2 次 CR3 切换 + IBRS + 寄存器清零）并没有消失，收益来自**一次 `io_uring_enter` 提交/收割 N 个 IO 的摊薄**。

</details>
</details>

**Q3.** `-ENOSYS` 是怎么产生的？内核里有几种实现方式？

<details><summary>答案</summary>

v6.6 有**两条完全不同的路径**：

**路径 A：号码被永久废弃 → 表里直填**

```c
/* kernel/sys_ni.c:22-25 */
asmlinkage long sys_ni_syscall(void)
{
	return -ENOSYS;
}
```

x86_64 `syscall_64.tbl` 里全表**只有 1 处**引用：

```
156	64	_sysctl			sys_ni_syscall
```

**路径 B：功能未编译进来 → 弱符号在链接期兜底**

```c
/* kernel/sys_ni.c:26 */
#define COND_SYSCALL(name) cond_syscall(sys_##name)
```

`sys_ni.c` 里有 **217 条** `COND_SYSCALL(...)`（io_uring、futex、clone3、epoll_pwait2、quotactl_fd……）。`cond_syscall()` 生成指向 `sys_ni_syscall` 的**弱符号别名**：若真正的实现因 `CONFIG_*` 关闭而没编译进 vmlinux，链接器就用弱符号填充。

| | 路径 A | 路径 B |
|---|-------|-------|
| 触发 | syscall 彻底移除 | syscall 存在但依赖的 CONFIG 关闭 |
| 位置 | `syscall_64.tbl` 表项 | `kernel/sys_ni.c` |
| 时机 | 编译表时 | **链接期** |
| v6.6 数量（x86_64） | **1 个** | **217 个** |

**意义**：这层弱符号兜底是"**内核可裁剪**"的基础设施——关掉 io_uring / fanotify / kexec 等功能不会让号码报错崩溃，而是优雅返回 `-ENOSYS`，glibc 据此退化到老实现。

</details>

**Q4.** 一次 syscall 的开销主要花在哪？为什么说"少做 syscall"比"让 syscall 更快"更有效？

<details><summary>答案</summary>

读 v6.6 `arch/x86/entry/entry_64.S:87-227` 的 `entry_SYSCALL_64`，开销清单如下：

| 步骤 | 指令/宏 | 成本性质 |
|------|--------|---------|
| 换 GS 基址 | `swapgs`（×2，进出各一次） | 中段 |
| ⭐ **换页表** | `SWITCH_TO_KERNEL_CR3` / `SWITCH_TO_USER_CR3_STACK` | **最贵**——写 CR3 可能触发 TLB 失效；Meltdown 的 PTI 代价 |
| 切栈 ×2 | `pcpu_hot + X86_top_of_stack`、`TSS_sp0` trampoline | 小段 |
| 构造 `pt_regs` | 6 次 `pushq` | 中段 |
| 寄存器压栈清零 | `PUSH_AND_CLEAR_REGS` / `POP_REGS` | 中段；防 Spectre 寄存器残留泄露 |
| ⭐ Spectre v2 防护 | `IBRS_ENTER` / `IBRS_EXIT` | **高** |
| ⭐ Retbleed 防护 | `UNTRAIN_RET` | **高** |
| 清栈 | `STACKLEAK_ERASE_NOCLOBBER` | 中高；防栈内容泄露 |

⭐⭐ **核心认知**：开销**主要不是"特权级切换"本身**，而是 **2018 年 Spectre/Meltdown 之后叠加的一整套侧信道缓解**——PTI（双页表）、IBRS、UNTRAIN_RET、寄存器清零、栈清空。这些都是**固定成本**，与 syscall 干什么活无关。

因此：
- **"让 syscall 更快"** = 削固定成本，但 ABI 兼容和安全底线下可削空间很小
- **"少做 syscall"** = 摊薄固定成本，收益是线性的

io_uring 走的就是第二条路：一次 `io_uring_enter` 提交/收割 N 个 IO，把固定成本除以 N。

</details>

**Q5.** vDSO 是什么？它导出了哪些符号？`clock_gettime` 为什么能在用户态算出时间？

<details><summary>答案</summary>

**vDSO（virtual Dynamic Shared Object）** = 内核把一小段代码 + 一个数据页**只读映射**到每个进程的用户地址空间，让部分"系统调用"完全在用户态执行，**零特权切换**。

v6.6 x86_64 导出的全部符号（`arch/x86/entry/vdso/vdso.lds.S` 的 VERSION 段，`LINUX_2.6` 版本节点）：

| 符号 | 作用 |
|------|------|
| `clock_gettime` / `__vdso_clock_gettime` | 高精度时间 |
| `gettimeofday` / `__vdso_gettimeofday` | 墙上时间 |
| `getcpu` / `__vdso_getcpu` | 当前 CPU / NUMA 节点 |
| `time` / `__vdso_time` | 秒级时间 |
| `clock_getres` / `__vdso_clock_getres` | 时钟分辨率 |
| `__vdso_sgx_enter_enclave` | 条件编译（`CONFIG_X86_SGX`） |

**为什么能在用户态算出时间：**

```c
/* lib/vdso/gettimeofday.c:266-269 */
__cvdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts)
{
	return __cvdso_clock_gettime_data(__arch_get_vdso_data(), clock, ts);
}
```

1. 内核的 timekeeping 代码**每 tick 更新**一个 `vdso_data` 页（含上次更新的时刻 + 插值所需的系数）
2. 这个页**只读映射**到每个进程
3. vDSO 代码读 `vdso_data`，用**纯算术插值**算出当前纳秒数
4. 并发保护用 ⭐ **seqcount**：用户态读序号，若读到奇数（= 内核正在更新）就重试——**无锁、无原子操作**

**安全性**：用户态只能读，改不了内核时间。

⚠️ **不是所有时钟都走 vDSO**：`CLOCK_MONOTONIC`、`CLOCK_REALTIME`、`CLOCK_BOOTTIME`、`CLOCK_MONOTONIC_RAW` 等纯计算时钟可以；复杂的会**回退到真 syscall**（vDSO 内部自己发 `syscall` 兜底）。

**HFT 含义**：走 vDSO ≈ **20-30 cycles**，走 syscall ≈ **1000+ cycles**，差 1~2 个数量级。打时间戳必须用 `clock_gettime(CLOCK_MONOTONIC)`。

</details>

**Q6.** `MAX_ERRNO` 为什么是 4095？内核怎么区分"合法指针"和"错误码"？

<details><summary>答案</summary>

有些 syscall 成功时返回**地址**（如 `mmap`），没法用"负数"表示错误。内核的编码方案（v6.6 `include/linux/err.h:18-49`）：

```c
#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)
static inline long __must_check PTR_ERR(__force const void *ptr) { return (long) ptr; }
```

**原理——利用地址空间的最后一页：**

```
0xFFFFFFFFFFFFFFFF  ┌─────────────────────┐
                    │  错误指针区间        │  0xFFFFFFFFFFFFF001 ~ ...FFFF
0xFFFFFFFFFFFFF001  │  -1 ~ -4095         │  ← 最后一页，永不映射
                    ├─────────────────────┤
                    │  合法地址空间        │  ← TASK_SIZE 以下才可能映射
0x0000000000000000  └─────────────────────┘
```

- 错误码 `-1 ~ -4095` 编码成 `0xFFFFFFFFFFFFF001 ~ 0xFFFFFFFFFFFFFFFF`
- 用户态可映射的最大地址 < `0xFFFFFFFFFFFFF000`（最后一页永不映射）
- ⟹ **与所有合法指针天然不冲突**

| 宏 | 作用 |
|----|------|
| `IS_ERR(p)` | 判断是否为错误指针：`(unsigned long)(p) >= -4095` |
| `PTR_ERR(p)` | 取出负错误码：`(long)p` |
| `ERR_PTR(e)` | 把负错误码编码成指针 |

**为什么是 4095 而不是别的数**：正好等于"一页 - 1"，覆盖地址空间最后一页。判断只需**一次无符号比较**，零成本。

| syscall 类别 | 失败返回 | 用户态判定 |
|-------------|---------|-----------|
| 返回 `long`（read/write） | `-errno` | `if (ret < 0)` |
| 返回指针（mmap） | `ERR_PTR(-errno)` | `IS_ERR(p)`；glibc 转成 `-1` + `errno` |

</details>

**Q7.** `asmlinkage` 的作用是什么？它真的让参数"从栈取"吗？

<details><summary>答案</summary>

⚠️ **"参数从栈取"只对 i386 成立，在 x86_64 上 `asmlinkage` 是空宏。**

**x86 的定义**（v6.6 `arch/x86/include/asm/linkage.h:19-21`）：

```c
#ifdef CONFIG_X86_32
#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))
#endif /* CONFIG_X86_32 */
```

注意包在 `#ifdef CONFIG_X86_32` 里 —— **64 位下这个宏根本不存在**，于是落到通用定义（`include/linux/linkage.h:14-22`）：

```c
#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif
```

| 架构/配置 | 展开结果 | 效果 |
|----------|---------|------|
| **x86_64** | **空** | ⭐ 毫无作用，纯历史遗迹 |
| **x86_32** | `extern "C" __attribute__((regparm(0)))` | 强制参数全部走栈 |
| C++ 编译 | `extern "C"` | 关闭 name mangling |
| 多数其他架构 | 空 | 同 x86_64 |

**为什么 i386 需要而 x86_64 不需要**：
- i386 只有 8 个通用寄存器，`regparm`/`fastcall` 之争最终让内核选"参数全走栈"以简化汇编入口
- x86_64 有 16 个寄存器，ABI 本来就用寄存器传参（`rdi`/`rsi`/`rdx`/`r10`/`r8`/`r9`），**不需要也不存在 `regparm(0)`**

**结论**：现代 x86_64 内核里 `asmlinkage` 只是**语义标注**（告诉读者"这个函数从汇编入口被调用"），编译层面什么都不做。

</details>

**Q8.** x86_64 的 syscall 寄存器约定是什么？写内联汇编时最容易踩什么坑？

<details><summary>答案</summary>

| 用途 | 寄存器 |
|------|--------|
| 系统调用号 | `rax` |
| 参数 1 | `rdi` |
| 参数 2 | `rsi` |
| 参数 3 | `rdx` |
| ⭐ **参数 4** | **`r10`**（不是 `rcx`！） |
| 参数 5 | `r8` |
| 参数 6 | `r9` |
| 返回值 | `rax` |
| **`syscall` 指令破坏** | `rcx`（存返回 RIP）、`r11`（存 RFLAGS） |

⚠️ **最常见的坑**：把第 4 个参数放进 `rcx`。

这是 x86_64 **syscall ABI 与 System V 函数调用 ABI 的唯一差异**——普通函数调用第 4 参数用 `rcx`，但 `syscall` 指令自身要用 `rcx` 保存返回地址，所以内核 ABI 改用 `r10`。

**安全性细节**：v6.6 `entry_64.S:115-116` 对号码做符号扩展：

```asm
	/* Sign extend the lower 32bit as syscall numbers are treated as int */
	movslq	%eax, %rsi
```

只取 `eax` 的低 32 位并**符号扩展**成 64 位。因此负数号码会变成巨大正数索引，在 `do_syscall_64` 的边界检查（`nr < NR_syscalls`）处被拒，**不可能索引到表外**。

</details>

**Q9.** 号码表上 335–423 这段空洞是怎么来的？

<details><summary>答案</summary>

**成因**：为 x32 ABI 预留。

v6.6 `syscall_64.tbl` 表头注释：

```
# don't use numbers 387 through 423, add new calls after the last
# 'common' entry
#
# Due to a historical design error, certain syscalls are numbered differently
# in x32 as compared to native x86_64.  These syscalls have numbers 512-547.
# Do not add new syscalls to this range.  Numbers 548 and above are available
# for non-x32 use.
```

**真正原因记录在 v5.1 的表头**（v6.6 已删除该句）：

> `# x32-specific system call numbers start at 512 to avoid cache impact for native 64-bit operation.`

即：syscall 表是**按号索引的连续数组**。如果 x32 的号码与原生 64 位号码交错分布，表的跨度会变大，恶化原生路径的 cache 局部性。所以给 x32 单独划了 512-547 段，中间留下 335–423 共 **89 个号码的空洞**。

**版本断崖**：号码跳跃发生在 **v5.1**（v5.0 最大号 334 `rseq` → v5.1 最大号 427）。v5.1 新增的四个号码是 424–427：

```
424	common	pidfd_send_signal
425	common	io_uring_setup
426	common	io_uring_enter
427	common	io_uring_register
```

⭐ **io_uring 是历史上第一批拿到 424+ 号码的 syscall**。

完整分析见 [Ch 1.1 Unix 的历史](../../chapter-01-intro/notes/section-1.1-Unix-的历史.md)。

</details>

**Q10.** 从 ABI 稳定性角度，v6.6 内核为了"永不破坏用户态"做了哪些兜底设计？

<details><summary>答案</summary>

至少五层：

| 机制 | 保护什么 |
|------|---------|
| ⭐ **号码永不回收** | 废弃 syscall 的槽位填 `sys_ni_syscall`（返回 `-ENOSYS`），而不是让后续 syscall 前移 |
| ⭐ **弱符号兜底** | 217 个 `cond_syscall` 让未编译的功能优雅返回 `-ENOSYS`，而非链接失败 |
| **compat 层** | 32 位程序在 64 位内核上运行（x32 / i386 的 `compat_sys_*`） |
| **`__x64_sys_*` 包装层** | `SYSCALL_DEFINE` 宏自动生成参数扩展/符号扩展，屏蔽 ABI 细节 |
| **号码符号扩展** | `movslq %eax, %rsi` 保证非法号码被边界检查拒绝 |

**为什么这么谨慎**：syscall 是**内核与用户态之间最强的 ABI 契约**。旧二进制程序（甚至 20 年前的静态链接程序）必须能在新内核上原样运行。Linus 的名言式原则就是"We do not break userspace"——任何导致用户程序行为变化的改动，即使用户程序本身"写得不对"，也算内核 regression。

**代价**：号码表碎片化（335–423 空洞）、每个新 syscall 要改所有架构的表、废弃功能只能占位不能清理。

</details>

</details>

---

> ↔ [ULK Ch10 §2 POSIX-API与系统调用](../../../16-linux-kernel-deep/chapter-10-system-calls/notes/section-2-POSIX-API与系统调用.md)
---
