## ③ Oops

> 承接 [18.2 通过打印调试](./section-18.2-通过打印调试.md)，下接 [18.4 内核调试选项](./section-18.4-内核调试选项.md)。
> 本节回答：**内核态出错时到底发生了什么、那一大坨输出每个字段是什么意思、怎么从它反推到源码行。**

#### Oops 的一句话定义

| | |
|---|---|
| **Oops** | 内核检测到**自己**犯了无法在本地修复的错误（空指针解引用、非法指令、页表损坏…），**打印现场**并**终止当前上下文** |
| ⚠️ **不是什么** | 不是"用户态 segfault 的内核版"。用户态缺页走 `do_user_addr_fault()` 发信号，**压根不进 Oops 路径** |

**HFT：** 自研低延迟驱动 / 内核模块 Oops 一次 = 一次交易中断。**看不懂 Oops = 排障全靠猜**。

---

### 一、⭐ 内核态缺页 ≠ Oops：Oops 之前有三道拦截网

这是最常被跳过的一层。很多人以为"内核里踩了坏地址就 Oops"——**不成立**。

v6.6 `arch/x86/mm/fault.c` 的 `kernelmode_fixup_or_oops()` 是内核态缺页的总入口：

```c
static noinline void
kernelmode_fixup_or_oops(struct pt_regs *regs, unsigned long error_code,
			 unsigned long address, int signal, int si_code,
			 u32 pkey)
{
	WARN_ON_ONCE(user_mode(regs));

	/* Are we prepared to handle this kernel fault? */
	if (fixup_exception(regs, X86_TRAP_PF, error_code, address)) {
		/*
		 * Any interrupt that takes a fault gets the fixup. This makes
		 * the below recursive fault logic only apply to a faults from
		 * task context.
		 */
		if (in_interrupt())
			return;
		...
		/* Barring that, we can do the fixup and be happy. */
		return;
	}

	/*
	 * AMD erratum #91 manifests as a spurious page fault on a PREFETCH
	 * instruction.
	 */
	if (is_prefetch(regs, error_code, address))
		return;

	page_fault_oops(regs, error_code, address);
}
```

三条出口：

| 顺序 | 机制 | 命中条件 | 结果 |
|---|---|---|---|
| ① | ⭐ **exception table**（`fixup_exception()`） | 出错指令地址在 `__ex_table` 段里有登记 | **跳到修复代码继续执行，完全不 Oops** |
| ② | `is_prefetch()` | 出错指令是 `PREFETCH`（**AMD erratum #91** 伪缺页） | 直接忽略返回 |
| ③ | `page_fault_oops()` | 以上都不命中 | **这才是 Oops** |

> ⭐⭐ **`copy_from_user()` / `get_user()` 为什么不会 Oops？** 答案就在 ①。
> 编译器为每条用户态访问指令在 `__ex_table` 里登记一条「出错地址 → 修复标号」，
> `fixup_exception()` 查表命中后直接改 `regs->ip`，跳到 `.Lbad_...` 标号返回 `-EFAULT`。
> **这是"内核能安全地碰用户指针"的全部基础**，也是第 5 章里系统调用取参不会崩的原因。

#### `page_fault_oops()` 内部还有两道

```c
page_fault_oops(struct pt_regs *regs, unsigned long error_code, unsigned long address)
{
	...
	if (user_mode(regs))
		goto oops;              /* 隐式内核访问：跳过栈溢出 / EFI 特例 */

#ifdef CONFIG_VMAP_STACK
	/* Stack overflow? ... */
	if (is_vmalloc_addr((void *)address) &&
	    get_stack_guard_info((void *)address, &info)) {
		/*
		 * We're likely to be running with very little stack space
		 * left. It's plausible that we'd hit this condition but
		 * double-fault even before we get this far, in which case
		 * we're fine: the double-fault handler will deal with it.
		 *
		 * We don't want to make it all the way into the oops code
		 * and then double-fault, though, because we're likely to
		 * break the console driver and lose most of the stack dump.
		 */
		call_on_stack(__this_cpu_ist_top_va(DF) - sizeof(void*),
			      handle_stack_overflow, ASM_CALL_ARG3,
			      , [arg1] "r" (regs), [arg2] "r" (address), [arg3] "r" (&info));
		unreachable();
	}
#endif
	/* Buggy firmware could access regions which might page fault. */
	if (IS_ENABLED(CONFIG_EFI))
		efi_crash_gracefully_on_page_fault(address);

	/* Only not-present faults should be handled by KFENCE. */
	if (!(error_code & X86_PF_PROT) &&
	    kfence_handle_page_fault(address, error_code & X86_PF_WRITE, regs))
		return;

oops:
	flags = oops_begin();

	show_fault_oops(regs, error_code, address);

	if (task_stack_end_corrupted(current))
		printk(KERN_EMERG "Thread overran stack, or stack corrupted\n");

	sig = SIGKILL;
	if (__die("Oops", regs, error_code))
		sig = 0;

	/* Executive summary in case the body of the oops scrolled away */
	printk(KERN_DEFAULT "CR2: %016lx\n", address);

	oops_end(flags, regs, sig);
}
```

⭐⭐ **栈溢出时先换栈、再打印** —— 全篇最"工程师"的一段代码：

> 内核栈（v6.6 默认 16KB）已经踩穿了，`page_fault_oops()` 自己再压几个栈帧就会 **double fault**。
> 所以源码先检测 `is_vmalloc_addr() && get_stack_guard_info()`（命中 guard page），
> 再用 `call_on_stack()` **把执行切到 Double-Fault 的 IST 栈**（`__this_cpu_ist_top_va(DF)`）上打印。
> 注释原话：*"We don't want to make it all the way into the oops code and then double-fault,
> because we're likely to break the console driver and lose most of the stack dump."*

| 拦截网 | 引入版本 | 作用 |
|--------|---------|------|
| exception table | 史前（v0.x） | 用户访问 / 可恢复故障 |
| `is_prefetch` | — | AMD erratum #91 伪缺页 |
| ⭐ `CONFIG_VMAP_STACK` 栈溢出换栈 | v4.9（x86） | 踩穿栈也能打出 backtrace |
| ⭐ **KFENCE** | v5.12 | 采样式内存错误检测，**抢在 Oops 之前**把 use-after-free / 越界变成结构化报告 |
| EFI 特殊路径 | — | 固件踩坏地址时优雅降级 |

---

### 二、Oops 的三段式协议：`oops_begin` → `__die` → `oops_end`

v6.6 `arch/x86/kernel/dumpstack.c` 里 `die()` 全文只有 **9 行**：

```c
/*
 * This is gone through when something in the kernel has done something bad
 * and is about to be terminated:
 */
void die(const char *str, struct pt_regs *regs, long err)
{
	unsigned long flags = oops_begin();
	int sig = SIGSEGV;

	if (__die(str, regs, err))
		sig = 0;
	oops_end(flags, regs, sig);
}
```

#### 第一段：`oops_begin()` — 抢锁、防嵌套、开控制台

```c
static arch_spinlock_t die_lock = __ARCH_SPIN_LOCK_UNLOCKED;
static int die_owner = -1;
static unsigned int die_nest_count;

unsigned long oops_begin(void)
{
	int cpu;
	unsigned long flags;

	oops_enter();

	/* racy, but better than risking deadlock. */
	raw_local_irq_save(flags);
	cpu = smp_processor_id();
	if (!arch_spin_trylock(&die_lock)) {
		if (cpu == die_owner)
			/* nested oops. should stop eventually */;
		else
			arch_spin_lock(&die_lock);
	}
	die_nest_count++;
	die_owner = cpu;
	console_verbose();
	bust_spinlocks(1);
	return flags;
}
```

| 动作 | 为什么 |
|------|--------|
| `oops_enter()` | 架构无关部分（见下） |
| `raw_local_irq_save()` | 打印期间禁本地中断，避免输出交错 |
| ⭐ `arch_spin_trylock` + `die_owner` | **嵌套 Oops 检测**：若持锁者就是本 CPU，说明打印过程中又炸了一次 → **不再** `arch_spin_lock`（否则死锁）。注释：`/* nested oops. should stop eventually */` |
| ⭐ `die_nest_count++` | 嵌套计数，**只有归零才真正放锁** |
| `console_verbose()` | 把 console loglevel 拉到最高，保证 Oops 真能打出来 |
| ⭐ `bust_spinlocks(1)` | 见下 |

`oops_enter()` 是架构无关部分（`kernel/panic.c`）：

```c
void oops_enter(void)
{
	tracing_off();
	/* can't trust the integrity of the kernel anymore: */
	debug_locks_off();
	do_oops_enter_exit();

	if (sysctl_oops_all_cpu_backtrace)
		trigger_all_cpu_backtrace();
}
```

| 动作 | 为什么 |
|------|--------|
| ⭐ `tracing_off()` | **先关 ftrace**——否则 Oops 打印本身会被 trace，可能递归 |
| ⭐ `debug_locks_off()` | 注释原话 *"can't trust the integrity of the kernel anymore"*：内核状态已损坏，lockdep 再报锁问题只会是噪音 |
| `do_oops_enter_exit()` | `pause_on_oops` 串行化（见下） |
| `sysctl_oops_all_cpu_backtrace` | `/proc/sys/kernel/oops_all_cpu_backtrace=1` → 顺便打**所有 CPU** 的栈 |

> ⭐ `bust_spinlocks(1)`：**"if a spinlock is held, break it"**。
> Oops 要往控制台打印，而控制台自己有自旋锁——如果正好崩在持有 console 锁的路径上，
> 打印就会死锁。所以 `bust_spinlocks()` 让 console 层的锁超时后**强行放行**。
> `oops_end()` 里用 `bust_spinlocks(0)` 恢复。

#### `pause_on_oops`：多 CPU 同时 Oops 时只让第一个打印

```c
static void do_oops_enter_exit(void)
{
	unsigned long flags;
	static int spin_counter;

	if (!pause_on_oops)
		return;

	spin_lock_irqsave(&pause_on_oops_lock, flags);
	if (pause_on_oops_flag == 0) {
		/* This CPU may now print the oops message */
		pause_on_oops_flag = 1;
	} else {
		/* We need to stall this CPU */
		...
	}
	spin_unlock_irqrestore(&pause_on_oops_lock, flags);
}

bool oops_may_print(void)
{
	return pause_on_oops_flag == 0;
}
```

`oops_may_print()` 就是**拦截点**——`show_fault_oops()` 第一行便是 `if (!oops_may_print()) return;`。

> ⭐ 设计目的（`oops_enter` 上方注释原话）：
> **"We do all this to ensure that oopses don't scroll off the screen."**
> 八核同时炸，八个 backtrace 交错输出 = 什么也看不清。所以第一个 CPU 打印，
> 其他 CPU **空转等待** `pause_on_oops` 秒。命令行：`pause_on_oops=<秒数>`。
>
> 源码还有一句诚实的吐槽："It turns out that the CPU which is allowed to print ends up
> pausing for the right duration, whereas all the other CPUs pause for **twice as long**:
> once in `oops_enter()`, once in `oops_exit()`."

#### 第二段：`__die()` — 真正打印

```c
int __die(const char *str, struct pt_regs *regs, long err)
{
	__die_header(str, regs, err);
	return __die_body(str, regs, err);
}

static void __die_header(const char *str, struct pt_regs *regs, long err)
{
	const char *pr = "";

	/* Save the regs of the first oops for the executive summary later. */
	if (!die_counter)
		exec_summary_regs = *regs;

	if (IS_ENABLED(CONFIG_PREEMPTION))
		pr = IS_ENABLED(CONFIG_PREEMPT_RT) ? " PREEMPT_RT" : " PREEMPT";

	printk(KERN_DEFAULT
	       "%s: %04lx [#%d]%s%s%s%s%s\n", str, err & 0xffff, ++die_counter,
	       pr,
	       IS_ENABLED(CONFIG_SMP)     ? " SMP"             : "",
	       debug_pagealloc_enabled()  ? " DEBUG_PAGEALLOC" : "",
	       IS_ENABLED(CONFIG_KASAN)   ? " KASAN"           : "",
	       IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION) ?
	       (boot_cpu_has(X86_FEATURE_PTI) ? " PTI" : " NOPTI") : "");
}

static int __die_body(const char *str, struct pt_regs *regs, long err)
{
	show_regs(regs);
	print_modules();

	if (notify_die(DIE_OOPS, str, regs, err,
			current->thread.trap_nr, SIGSEGV) == NOTIFY_STOP)
		return 1;

	return 0;
}
```

| 关键点 | 说明 |
|--------|------|
| `%04lx` | `err & 0xffff` = 硬件 **error code**，就是 `Oops: 0002 [#1]` 里的 `0002` |
| `[#%d]` | **同一次 boot 内的 Oops 计数**（`++die_counter`）。`[#1]` = 第一次 |
| ⭐ `exec_summary_regs` | **只存第一次 Oops 的寄存器**（`if (!die_counter)`），供 `oops_end()` 末尾的 "executive summary" 用 |
| `PREEMPT` / `PREEMPT_RT` | 抢占模型标记（**HFT 必看**：RT 内核行为完全不同） |
| `PTI` / `NOPTI` | 页表隔离是否真的生效（Meltdown 缓解） |
| ⭐ `notify_die(DIE_OOPS, ...)` | **kprobe / kgdb / kdump 的挂钩点**。返回 `NOTIFY_STOP` → `__die()` 返回 1 → `die()` 里 `sig = 0` → `oops_end()` **直接 return 不杀进程**（调试器接管了） |

#### 第三段：`oops_end()` — 收尾与三种下场

```c
void oops_end(unsigned long flags, struct pt_regs *regs, int signr)
{
	if (regs && kexec_should_crash(current))
		crash_kexec(regs);

	bust_spinlocks(0);
	die_owner = -1;
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	die_nest_count--;
	if (!die_nest_count)
		/* Nest count reaches zero, release the lock. */
		arch_spin_unlock(&die_lock);
	raw_local_irq_restore(flags);
	oops_exit();

	/* Executive summary in case the oops scrolled away */
	__show_regs(&exec_summary_regs, SHOW_REGS_ALL, KERN_DEFAULT);

	if (!signr)
		return;
	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");

	/*
	 * We're not going to return, but we might be on an IST stack or
	 * have very little stack space left.  Rewind the stack and kill
	 * the task.
	 * Before we rewind the stack, we have to tell KASAN that we're going to
	 * reuse the task stack and that existing poisons are invalid.
	 */
	kasan_unpoison_task_stack(current);
	rewind_stack_and_make_dead(signr);
}
```

| 步骤 | 说明 |
|------|------|
| ⭐ `crash_kexec(regs)` | **kdump 的入口**——`kexec_should_crash(current)` 判定（可用 `CRASH_*` 标志限制只在特定进程/中断上下文触发） |
| `bust_spinlocks(0)` | 恢复 console 锁语义 |
| ⭐ `add_taint(TAINT_DIE, ...)` | **Oops 之后内核被标记 `D` 污染**——这是后面 `panic()` 决定要不要再打一次栈的依据 |
| `die_nest_count--` | 归零才放 `die_lock` |
| `oops_exit()` | 见下 |
| ⭐ 二次打印 `exec_summary_regs` | 注释：`Executive summary in case the oops scrolled away` —— **正文被刷屏冲掉后，末尾还有一份精简版** |
| ⭐ `rewind_stack_and_make_dead()` | **不是普通 `do_exit()`**！注释解释：此时可能站在 **IST 栈**上或栈所剩无几，必须先"回卷栈"再杀进程 |

```c
void oops_exit(void)
{
	do_oops_enter_exit();
	print_oops_end_marker();   /* pr_warn("---[ end trace %016llx ]---\n", 0ULL); */
	kmsg_dump(KMSG_DUMP_OOPS);
}
```

> ⭐⭐ `kmsg_dump(KMSG_DUMP_OOPS)` 是 **pstore / ramoops / netconsole 的落盘点**。
> 没有它，Oops 只存在于 dmesg 里——**重启即失**。
> 嵌入式设备没有串口控制台时，配 `pstore` + `ramoops` 是抓 Oops 的唯一办法。

#### Oops 之后进程的三种下场

| 条件 | 结果 | 源码 |
|------|------|------|
| `signr == 0`（调试器 `NOTIFY_STOP`） | **不杀进程，继续跑** | `if (!signr) return;` |
| ⭐ `in_interrupt()` | **`panic("Fatal exception in interrupt")`** | 中断上下文没有"当前进程"可杀 |
| `panic_on_oops` 非 0 | **`panic("Fatal exception")`** | 生产环境常见配置 |
| 都不满足 | ⭐ **`rewind_stack_and_make_dead(signr)`** | 杀掉当前进程，内核继续 |

> ⚠️ 常见误解：**"Oops 在普通进程上下文就不会 panic"**——准确说法是"不会**立刻** panic"。
> 内核数据结构可能已被破坏（比如崩在持有某把锁的临界区里），
> 后续会出现**症状与根因分离的二次故障**。所以 HFT/生产环境普遍设 `panic_on_oops=1`。

---

### 三、Oops 输出逐行解剖（v6.6 真实格式）

#### 完整样例（v6.6 实际会打印的形状）

```
BUG: kernel NULL pointer dereference, address: 0000000000000010
#PF: supervisor write access in kernel mode
#PF: error_code(0x0002) - not-present page
PGD 0 P4D 0
Oops: 0002 [#1] PREEMPT SMP PTI
CPU: 3 PID: 4242 Comm: mytest Tainted: G                  D      6.6.0 #1
Hardware name: ...
RIP: 0010:my_drv_ioctl+0x42/0x100 [mydrv]
Code: 48 8b 7f 08 48 85 ff 74 0a 48 8b 07 c3 90 <48> 89 78 10 31 c0 c3 90 90 66 2e ...
RSP: 0018:ffffc90000123d50 EFLAGS: 00010286
RAX: 0000000000000000 RBX: ffff888012345678 RCX: 0000000000000000
RDX: 0000000000000000 RSI: 0000000000000000 RDI: 0000000000000000
...
CR2: 0000000000000010
---[ end trace 0000000000000000 ]---
```

#### 3.1 第一行 `BUG:` —— 而且 v5.2 换过格式

v6.6 `show_fault_oops()`：

```c
	if (address < PAGE_SIZE && !user_mode(regs))
		pr_alert("BUG: kernel NULL pointer dereference, address: %px\n",
			(void *)address);
	else
		pr_alert("BUG: unable to handle page fault for address: %px\n",
			(void *)address);
```

> ⭐⭐ **判定条件是 `address < PAGE_SIZE`（4096），不是 `address == 0`**！
> 所以 `CR2: 0000000000000010` 也会报 **"kernel NULL pointer dereference"**。
> 这是**有意的设计**：`NULL->field` 是最常见的错误模式，小偏移一律按空指针归类。

**版本断崖（实测）：**

| 版本 | 输出格式 |
|------|---------|
| ≤ **v5.1** | `BUG: unable to handle kernel %s at %px`，其中 `%s` = `"NULL pointer dereference"` / `"paging request"`，后面跟 `[PROT][WRITE][USER][RSVD]` 方括号标记 |
| ⭐ **≥ v5.2** | `BUG: kernel NULL pointer dereference, address: %px` **或** `BUG: unable to handle page fault for address: %px`；错误码改由下面的 `#PF: error_code(...)` 行承担 |

老格式那套方括号标记的实现（v5.0 `fault.c`）：

```c
	pr_alert("BUG: unable to handle kernel %s at %px\n",
		 address < PAGE_SIZE ? "NULL pointer dereference" : "paging request",
		 (void *)address);

	err_txt[0] = 0;

	/*
	 * Note: length of these appended strings including the separation space and the
	 * zero delimiter must fit into err_txt[].
	 */
	err_str_append(error_code, err_txt, X86_PF_PROT,  "[PROT]" );
	err_str_append(error_code, err_txt, X86_PF_WRITE, "[WRITE]");
	err_str_append(error_code, err_txt, X86_PF_USER,  "[USER]" );
	err_str_append(error_code, err_txt, X86_PF_RSVD,  "[RSVD]" );
```

> ⚠️ **排障提示**：网上大量 Oops 教程用的是 **v5.1 及以前**的格式。
> 看到 `unable to handle kernel NULL pointer dereference at 0000000000000010`
> （没有逗号 + `address:`）说明那台机器跑的是 **5.1 之前**的内核——字段位置完全不同，别套错模板。
>
> 顺带一提：`address < PAGE_SIZE` 这个**判定逻辑从 v5.0 到 v6.6 一字未改**，只是换了措辞。

#### 3.2 `#PF:` 两行 + error_code 位表

```c
	pr_alert("#PF: %s %s in %s mode\n",
		 (error_code & X86_PF_USER)  ? "user" : "supervisor",
		 (error_code & X86_PF_INSTR) ? "instruction fetch" :
		 (error_code & X86_PF_WRITE) ? "write access" :
					       "read access",
			     user_mode(regs) ? "user" : "kernel");
	pr_alert("#PF: error_code(0x%04lx) - %s\n", error_code,
		 !(error_code & X86_PF_PROT) ? "not-present page" :
		 (error_code & X86_PF_RSVD)  ? "reserved bit violation" :
		 (error_code & X86_PF_PK)    ? "protection keys violation" :
					       "permissions violation");
```

| 位 | 名字 | 含义 |
|----|------|------|
| `0x01` | `X86_PF_PROT` | 0 = **页不存在**，1 = **权限违规** |
| `0x02` | `X86_PF_WRITE` | 0 = 读，1 = **写** |
| `0x04` | `X86_PF_USER` | 0 = supervisor（内核），1 = user |
| `0x08` | `X86_PF_RSVD` | **保留位被置位**（页表项里有非法位）→ 页表被写坏 |
| `0x10` | `X86_PF_INSTR` | **取指**（执行）时缺页 |
| `0x20` | `X86_PF_PK` | **保护键（MPK）** 违规 |

> ⭐ **诊断速查**：
> `error_code(0x0002)` = 写 + 页不存在 = **写空指针**（最常见）
> `error_code(0x0000)` = 读 + 页不存在 = **读空指针**
> `error_code(0x0009)` = 页不存在 + **保留位置位** = ⚠️ **页表已被破坏**，通常不是这个 Oops 的锅，要往前找

#### 3.3 安全告警：NX / SMEP 检测

`show_fault_oops()` 开头还有一段**攻击检测**：

```c
	if (error_code & X86_PF_INSTR) {
		pgd = __va(read_cr3_pa());
		pgd += pgd_index(address);
		pte = lookup_address_in_pgd(pgd, address, &level);

		if (pte && pte_present(*pte) && !pte_exec(*pte))
			pr_crit("kernel tried to execute NX-protected page - exploit attempt? (uid: %d)\n",
				from_kuid(&init_user_ns, current_uid()));
		if (pte && pte_present(*pte) && pte_exec(*pte) &&
				(pgd_flags(*pgd) & _PAGE_USER) &&
				(__read_cr4() & X86_CR4_SMEP))
			pr_crit("unable to execute userspace code (SMEP?) (uid: %d)\n",
				from_kuid(&init_user_ns, current_uid()));
	}
```

> ⭐ 只在 `X86_PF_INSTR`（取指缺页）时检查。看到这两行 = **不是普通 bug，是疑似漏洞利用**，优先级拉满。

#### 3.4 `Oops: 0002 [#1] PREEMPT SMP PTI` 行

即 `__die_header()` 的输出。结构：

```
<str>: <error_code 低 16 位> [#<第几次 Oops>] <抢占模型> <SMP> <DEBUG_PAGEALLOC> <KASAN> <PTI/NOPTI>
```

| 后缀 | 来源 | 含义 |
|------|------|------|
| `PREEMPT` / `PREEMPT_RT` | `CONFIG_PREEMPTION` | 抢占模型 |
| `SMP` | `CONFIG_SMP` | 对称多处理 |
| `DEBUG_PAGEALLOC` | `debug_pagealloc_enabled()` | 页分配调试 |
| `KASAN` | `CONFIG_KASAN` | 内存错误检测 |
| `PTI` / `NOPTI` | `CONFIG_PAGE_TABLE_ISOLATION` + `X86_FEATURE_PTI` | 页表隔离实际是否生效 |

#### 3.5 `Code:` 行 —— ⭐ 尖括号里那个字节就是凶手

```c
#define PROLOGUE_SIZE 42
#define EPILOGUE_SIZE 21
#define OPCODE_BUFSIZE (PROLOGUE_SIZE + 1 + EPILOGUE_SIZE)   /* 64 */

void show_opcodes(struct pt_regs *regs, const char *loglvl)
{
	u8 opcodes[OPCODE_BUFSIZE];
	unsigned long prologue = regs->ip - PROLOGUE_SIZE;

	switch (copy_code(regs, opcodes, prologue, sizeof(opcodes))) {
	case 0:
		printk("%sCode: %" __stringify(PROLOGUE_SIZE) "ph <%02x> %"
		       __stringify(EPILOGUE_SIZE) "ph\n", loglvl, opcodes,
		       opcodes[PROLOGUE_SIZE], opcodes + PROLOGUE_SIZE + 1);
		break;
	case -EPERM:
		/* No access to the user space stack of other tasks. Ignore. */
		break;
	default:
		printk("%sCode: Unable to access opcode bytes at 0x%lx.\n",
		       loglvl, prologue);
		break;
	}
}
```

⭐⭐ **`Code:` 行 = 42 字节前缀 + `<出错字节>` + 21 字节后缀。尖括号包住的那一个字节，就是 RIP 指向的指令首字节。**

为什么是 42/21 这种奇怪比例？源码注释（courtesy of Linus）：

> - 没有精确内核映像时，更大的前缀便于**跨 toolchain 比对代码**
> - 帮助**重建出错内核的寄存器分配**
> - ⭐ 最重要的是：x86 是**变长指令**架构，需要足够长的前缀让反汇编器 **"sync up properly and find instruction boundaries"**
> - 收尾一句很坦白：*"the 2/3rds prologue and 64 byte OPCODE_BUFSIZE is just a **random guesstimate**"*

> ⭐ 实操：把 `Code:` 行的字节（去掉尖括号）喂给
> `objdump -D -b binary -m i386:x86-64` 就能反汇编，
> 找到 `<xx>` 那个字节对应的指令 = 出错指令。**没有 vmlinux 时这是唯一线索。**

#### 3.6 `show_regs()` / `__show_regs()` 完整字段

```c
void show_regs(struct pt_regs *regs)
{
	enum show_regs_mode print_kernel_regs;

	show_regs_print_info(KERN_DEFAULT);

	print_kernel_regs = user_mode(regs) ? SHOW_REGS_USER : SHOW_REGS_ALL;
	__show_regs(regs, print_kernel_regs, KERN_DEFAULT);

	/*
	 * When in-kernel, we also print out the stack at the time of the fault..
	 */
	if (!user_mode(regs))
		show_trace_log_lvl(current, regs, NULL, KERN_DEFAULT);
}
```

| 模式 | 触发 | 内容 |
|------|------|------|
| `SHOW_REGS_SHORT` | 内部调用 | 只打通用寄存器 |
| ⭐ `SHOW_REGS_USER` | `user_mode(regs)` | 通用寄存器 + FS/GS（**不读控制寄存器**） |
| `SHOW_REGS_ALL` | 内核态 Oops | 通用寄存器 + 段寄存器 + **CR0/CR2/CR3/CR4** + DR0-7 + PKRU |

`__show_regs()` 的细节（`arch/x86/kernel/process_64.c`）：

| 字段 | 打印条件 | 用途 |
|------|---------|------|
| ⭐ `ORIG_RAX` | **只在 `!= -1` 时打印** | ⭐ **系统调用号**！非系统调用上下文是 -1（呼应 5.3 节） |
| `RIP` + `CS` | 总是 | 出错指令 |
| `RSP` + `SS` + `EFLAGS` | 总是 | 栈与标志位 |
| `CR2` | `SHOW_REGS_ALL` | ⭐ **触发缺页的线性地址** |
| `CR3` | 同上 | 当前页表基址（**KASLR / PTI 排查用**） |
| ⭐ `DR0`-`DR7` | **只在非默认状态时打印** | 源码：`if (!((d0==0) && ... && (d6==DR6_RESERVED) && (d7==0x400)))` —— 有硬件断点时才会出现 |
| ⭐ `PKRU` | 只在 `X86_FEATURE_OSPKE` | 保护键寄存器（MPK） |

```c
	/* Only print out debug registers if they are in their non-default state. */
	if (!((d0 == 0) && (d1 == 0) && (d2 == 0) && (d3 == 0) &&
	    (d6 == DR6_RESERVED) && (d7 == 0x400))) {
		printk("%sDR0: %016lx DR1: %016lx DR2: %016lx\n", log_lvl, d0, d1, d2);
		printk("%sDR3: %016lx DR6: %016lx DR7: %016lx\n", log_lvl, d3, d6, d7);
	}
```

> ⭐⭐ **`CR2` 是最有价值的字段之一**：
> `CR2 = 0x10` + `RIP = my_drv_ioctl+0x42` ⇒ 在 `my_drv_ioctl` 里访问了某结构体偏移 `0x10` 的成员，而该结构体指针是 NULL。
> 用 `pahole -C my_dev mydrv.ko` 或 gdb 的 `p &((struct my_dev *)0)->xxx` 就能**反推是哪个字段**，
> 进而倒查"哪条路径会传 NULL 进来"。

#### 3.7 `Call Trace`

由 `show_trace_log_lvl()` 打印。v6.6 默认用 **ORC unwinder**（`CONFIG_UNWINDER_ORC`），不再依赖 frame pointer——
所以**不需要**为了拿到完整调用栈而关掉 `-fomit-frame-pointer`（这是 v4.x 时代的常见建议，已过时）。

---

### 四、⭐ Tainted 完整字母表（v6.6 共 19 位）

原笔记写的 "G = 专有模块" 是**反的**。看源码：

```c
const struct taint_flag taint_flags[TAINT_FLAGS_COUNT] = {
	[ TAINT_PROPRIETARY_MODULE ]	= { 'P', 'G', true },
	[ TAINT_FORCED_MODULE ]		= { 'F', ' ', true },
	[ TAINT_CPU_OUT_OF_SPEC ]	= { 'S', ' ', false },
	[ TAINT_FORCED_RMMOD ]		= { 'R', ' ', false },
	[ TAINT_MACHINE_CHECK ]		= { 'M', ' ', false },
	[ TAINT_BAD_PAGE ]		= { 'B', ' ', false },
	[ TAINT_USER ]			= { 'U', ' ', false },
	[ TAINT_DIE ]			= { 'D', ' ', false },
	[ TAINT_OVERRIDDEN_ACPI_TABLE ]	= { 'A', ' ', false },
	[ TAINT_WARN ]			= { 'W', ' ', false },
	[ TAINT_CRAP ]			= { 'C', ' ', true },
	[ TAINT_FIRMWARE_WORKAROUND ]	= { 'I', ' ', false },
	[ TAINT_OOT_MODULE ]		= { 'O', ' ', true },
	[ TAINT_UNSIGNED_MODULE ]	= { 'E', ' ', true },
	[ TAINT_SOFTLOCKUP ]		= { 'L', ' ', false },
	[ TAINT_LIVEPATCH ]		= { 'K', ' ', true },
	[ TAINT_AUX ]			= { 'X', ' ', true },
	[ TAINT_RANDSTRUCT ]		= { 'T', ' ', true },
	[ TAINT_TEST ]			= { 'N', ' ', true },
};

const char *print_tainted(void)
{
	static char buf[TAINT_FLAGS_COUNT + sizeof("Tainted: ")];

	BUILD_BUG_ON(ARRAY_SIZE(taint_flags) != TAINT_FLAGS_COUNT);

	if (tainted_mask) {
		char *s;
		int i;

		s = buf + sprintf(buf, "Tainted: ");
		for (i = 0; i < TAINT_FLAGS_COUNT; i++) {
			const struct taint_flag *t = &taint_flags[i];
			*s++ = test_bit(i, &tainted_mask) ?
					t->c_true : t->c_false;
		}
		*s = 0;
	} else
		snprintf(buf, sizeof(buf), "Not tainted");

	return buf;
}
```

⭐⭐ **输出是 19 个固定槽位，不是紧凑拼接。每一位对应一个固定位置；未置位打 `c_false`（大部分是空格）。**

所以 `Tainted: G...` 里位置 0 的 `G` 含义是：**「没有加载专有模块」**（`c_false`）。
**`P` 才表示加载了专有模块**（`c_true`）。原笔记说反了。

#### 全 19 位对照表

| # | 宏 | 置位 | 未置位 | 含义 | per-module |
|---|-----|------|-------|------|-----------|
| 0 | `TAINT_PROPRIETARY_MODULE` | ⭐ **P** | **G** | 加载了专有（非 GPL）模块 | ✅ |
| 1 | `TAINT_FORCED_MODULE` | F | 空格 | 用 `--force` 强载模块 | ✅ |
| 2 | `TAINT_CPU_OUT_OF_SPEC` | S | 空格 | CPU 跑在规格外（超频 / 过热） | ❌ |
| 3 | `TAINT_FORCED_RMMOD` | R | 空格 | 强制卸载模块 | ❌ |
| 4 | `TAINT_MACHINE_CHECK` | M | 空格 | 发生过 **MCE** | ❌ |
| 5 | `TAINT_BAD_PAGE` | B | 空格 | 释放了坏页 / 页表异常 | ❌ |
| 6 | `TAINT_USER` | U | 空格 | 用户**主动**写 `/proc/sys/kernel/tainted` | ❌ |
| 7 | ⭐ `TAINT_DIE` | **D** | 空格 | **发生过 Oops**（`oops_end()` 里加的） | ❌ |
| 8 | `TAINT_OVERRIDDEN_ACPI_TABLE` | A | 空格 | ACPI 表被覆盖 | ❌ |
| 9 | `TAINT_WARN` | **W** | 空格 | 触发过 `WARN()` | ❌ |
| 10 | `TAINT_CRAP` | C | 空格 | 加载了 **staging** 驱动 | ✅ |
| 11 | `TAINT_FIRMWARE_WORKAROUND` | I | 空格 | 绕过了固件 bug | ❌ |
| 12 | `TAINT_OOT_MODULE` | O | 空格 | 加载了**树外**模块 | ✅ |
| 13 | `TAINT_UNSIGNED_MODULE` | E | 空格 | 加载了**未签名**模块 | ✅ |
| 14 | `TAINT_SOFTLOCKUP` | L | 空格 | 发生过 **soft lockup** | ❌ |
| 15 | `TAINT_LIVEPATCH` | K | 空格 | 打过 **livepatch** | ✅ |
| 16 | `TAINT_AUX` | X | 空格 | 辅助标记（供外部工具） | ✅ |
| 17 | `TAINT_RANDSTRUCT` | T | 空格 | 启用了 `CONFIG_RANDSTRUCT` | ✅ |
| 18 | `TAINT_TEST` | N | 空格 | 加载了**测试用**模块 | ✅ |

```c
#define TAINT_FLAGS_COUNT		19
#define TAINT_FLAGS_MAX			((1UL << TAINT_FLAGS_COUNT) - 1)
```

> ⭐ **上游报 bug 的规矩**：tainted 的内核，maintainer **有权直接忽略**。
> 报之前先解释每个字母从哪来。
>
> ⭐⭐ 特别注意 **`D`**：打 `D` 说明**这台机器之前已经 Oops 过一次**——
> 那个**第一次** Oops 才是根因，后面的都可能是连锁反应。
> 这也正是 `__die_header()` 里 `if (!die_counter) exec_summary_regs = *regs;` 只存第一次的原因。

---

### 五、Oops → panic：`panic()` 的 14 步

#### 5.1 什么时候 Oops 变 panic

| 触发点 | 源码位置 | 说明 |
|--------|---------|------|
| `in_interrupt()` | `oops_end()` | **中断上下文没有进程可杀** |
| `panic_on_oops` 非 0 | `oops_end()` | 命令行 `oops=panic` |
| `pgtable_bad()` | `fault.c` | 页表损坏：`sig = SIGKILL` + `__die("Bad pagetable")` |
| 关键进程 | — | idle (pid 0) / init (pid 1) 崩了无法继续 |

```c
static int __init oops_setup(char *s)
{
	if (!s)
		return -EINVAL;
	if (!strcmp(s, "panic"))
		panic_on_oops = 1;
	return 0;
}
early_param("oops", oops_setup);
```

> ⚠️ **实测纠正**：`panic_on_oops` 的原型是 `int`（不是 bool），
> 但 **`oops_setup()` 只认 `"panic"` 一个字符串，只设成 1**。
> 网上流传的 `oops=3`（Oops 3 次后 panic）**在 v6.6 不存在**。
> 类似的计数机制只有 **`kernel.warn_limit`**（v6.2 引入），而且它作用于 `WARN()` 而非 Oops：
> ```c
> void check_panic_on_warn(const char *origin)
> {
> 	unsigned int limit;
>
> 	if (panic_on_warn)
> 		panic("%s: panic_on_warn set ...\n", origin);
>
> 	limit = READ_ONCE(warn_limit);
> 	if (atomic_inc_return(&warn_count) >= limit && limit)
> 		panic("%s: system warned too often (kernel.warn_limit is %d)",
> 		      origin, limit);
> }
> ```

#### 5.2 `panic()` 主流程（v6.6 `kernel/panic.c`）

| 步 | 动作 | 为什么 |
|----|------|--------|
| 1 | `local_irq_disable()` + `preempt_disable_notrace()` | 防止在设置 `panic_cpu` 之后被中断再次 `panic()` |
| 2 | ⭐ `atomic_cmpxchg(&panic_cpu, PANIC_CPU_INVALID, this_cpu)` | **只有第一个 CPU 执行 panic 主体**，其他 `panic_smp_self_stop()` |
| 3 | `console_verbose()` + `bust_spinlocks(1)` | 保证能打印 |
| 4 | `pr_emerg("Kernel panic - not syncing: %s\n", buf)` | |
| 5 | ⭐ `if (!test_taint(TAINT_DIE) && oops_in_progress <= 1) dump_stack();` | **避免重复打栈**：Oops→panic 时栈已打过 |
| 6 | ⭐ `kgdb_panic(buf)` | 注释：*"give it a chance to run before we stop all the other CPUs"*——否则其他 CPU 上的进程没法调试 |
| 7 | `if (!_crash_kexec_post_notifiers) __crash_kexec(NULL)` | kdump 默认**尽早**接管 |
| 8 | `panic_other_cpus_shutdown()` | `panic_print & PANIC_PRINT_ALL_CPU_BT` → `trigger_all_cpu_backtrace()`，然后 `smp_send_stop()` |
| 9 | `atomic_notifier_call_chain(&panic_notifier_list, ...)` | 第三方 panic handler |
| 10 | `panic_print_sys_info(false)` | 按位掩码打 task / mem / timer / lock / ftrace |
| 11 | ⭐ `kmsg_dump(KMSG_DUMP_PANIC)` | **pstore 落盘** |
| 12 | `if (_crash_kexec_post_notifiers) __crash_kexec(NULL)` | 延迟 kdump |
| 13 | `console_flush_on_panic(CONSOLE_FLUSH_PENDING)` | ⭐ 注释解释：可能**停掉了持有 console 锁的 CPU**，所以要抢锁再释放，把缓冲区吐出来 |
| 14 | `panic_timeout` 三态 | 见下 |

第 5 步的源码：

```c
	pr_emerg("Kernel panic - not syncing: %s\n", buf);
#ifdef CONFIG_DEBUG_BUGVERBOSE
	/*
	 * Avoid nested stack-dumping if a panic occurs during oops processing
	 */
	if (!test_taint(TAINT_DIE) && oops_in_progress <= 1)
		dump_stack();
#endif
```

> ⭐ `test_taint(TAINT_DIE)` 就是 **§4 里那个 `D`**——Oops 打过栈了，panic 就不再重复打。
> 两个机制的联动点就在这里。

#### ⭐ kdump 的两难：`crash_kexec_post_notifiers`

```
默认（=0）：__crash_kexec  ──►  停其他 CPU  ──►  panic notifiers  ──►  kmsg_dump
             └ 尽早抓 vmcore，最可靠

=1        ：停其他 CPU  ──►  panic notifiers  ──►  kmsg_dump  ──►  __crash_kexec
             └ 先拿到 dmesg / notifier 信息，但 notifier 可能让内核更不稳 → kdump 失败风险上升
```

源码注释原话：

> *"If you doubt kdump always works fine in any situation, `crash_kexec_post_notifiers`
> offers you a chance to run panic_notifiers and dumping kmsg before kdump.
> Note: since some panic_notifiers can make crashed kernel more unstable,
> it can increase risks of the kdump failure too."*

#### `panic_timeout` 的三态语义

| 值 | 行为 |
|----|------|
| `> 0` | 打印 `Rebooting in %d seconds..`，`mdelay` 循环等待后 `emergency_restart()` |
| `< 0` | **跳过等待，直接** `emergency_restart()`（因为 `panic_timeout != 0`） |
| `= 0` | ⭐ **不重启**：`suppress_printk = 1` + 无限 `mdelay(100)` + `panic_blink()`（键盘灯闪烁） |

```c
	/* Do not scroll important messages printed above */
	suppress_printk = 1;
	local_irq_enable();
	for (i = 0; ; i += PANIC_TIMER_STEP) {
		touch_softlockup_watchdog();
		if (i >= i_next) {
			i += panic_blink(state ^= 1);
			i_next = i + 3600 / PANIC_BLINK_SPD;
		}
		mdelay(PANIC_TIMER_STEP);
	}
```

> ⭐ `suppress_printk = 1` 的作用：源码注释 *"Do not scroll important messages printed above"*——
> panic 之后不再接受新打印，**把现场锁在屏幕上**。想重启只能人工按复位键或配 watchdog。
>
> `PANIC_TIMER_STEP 100`（ms）、`PANIC_BLINK_SPD 18`（每 200ms 翻一次 blink 状态）。

#### `panic_print` 位掩码（7 位）

```c
#define PANIC_PRINT_TASK_INFO		0x00000001
#define PANIC_PRINT_MEM_INFO		0x00000002
#define PANIC_PRINT_TIMER_INFO		0x00000004
#define PANIC_PRINT_LOCK_INFO		0x00000008
#define PANIC_PRINT_FTRACE_INFO		0x00000010
#define PANIC_PRINT_ALL_PRINTK_MSG	0x00000020
#define PANIC_PRINT_ALL_CPU_BT		0x00000040
```

用法：`panic_print=<掩码>` 内核参数。

| 位 | 效果 | 什么时候加 |
|----|------|-----------|
| `0x01` TASK | `show_state()` — 所有进程状态 | 怀疑某进程卡死 |
| `0x02` MEM | `show_mem()` | 怀疑内存耗尽 / 泄漏 |
| `0x04` TIMER | `sysrq_timer_list_show()` | 怀疑定时器问题 |
| `0x08` LOCK | `debug_show_all_locks()` | ⭐ **怀疑死锁** |
| `0x10` FTRACE | `ftrace_dump(DUMP_ALL)` | 需要完整 trace |
| `0x20` ALL_PRINTK | `console_flush_on_panic(CONSOLE_REPLAY_ALL)` | ⭐ **串口太慢丢日志时** |
| `0x40` ALL_CPU_BT | `trigger_all_cpu_backtrace()` | 需要其他 CPU 的现场 |

> ⭐ `panic_print_sys_info()` 会被调用**两次**（`false` / `true`）。
> 第二次带 `console_flush=true`，**只处理 `ALL_PRINTK_MSG`**——
> 因为那时 CPU 已停，可以安全地全量重放 printk 缓冲。
>
> ```c
> static void panic_print_sys_info(bool console_flush)
> {
> 	if (console_flush) {
> 		if (panic_print & PANIC_PRINT_ALL_PRINTK_MSG)
> 			console_flush_on_panic(CONSOLE_REPLAY_ALL);
> 		return;
> 	}
> 	if (panic_print & PANIC_PRINT_TASK_INFO)  show_state();
> 	if (panic_print & PANIC_PRINT_MEM_INFO)   show_mem();
> 	if (panic_print & PANIC_PRINT_TIMER_INFO) sysrq_timer_list_show();
> 	if (panic_print & PANIC_PRINT_LOCK_INFO)  debug_show_all_locks();
> 	if (panic_print & PANIC_PRINT_FTRACE_INFO) ftrace_dump(DUMP_ALL);
> }
> ```

---

### 六、解码工具链：ksymoops → kallsyms → decode_stacktrace.sh

| 时代 | 做法 | 痛点 |
|------|------|------|
| 2.4 及以前 | **`ksymoops`** + `System.map` 手工比对 | 要人工喂 `System.map` 和模块加载地址 |
| ⭐ 2.6+ | **`CONFIG_KALLSYMS`** — 符号表编进内核 | Oops **直接可读** |
| 现代 | **`scripts/decode_stacktrace.sh`** | 一键把整份 Oops 转成「文件:行号」 |
| 重度 | **kdump + crash** | 交互式 `bt` / `struct` / `rd` |

`decode_stacktrace.sh`（v6.6）的能力：

```bash
# 用法
./scripts/decode_stacktrace.sh -r 6.6.0                     # 自动找 vmlinux
./scripts/decode_stacktrace.sh vmlinux <base path> <modpath>
# 典型
dmesg | ./scripts/decode_stacktrace.sh vmlinux auto /lib/modules/$(uname -r)
```

| 特性 | 说明 |
|------|------|
| `-r <release>` | 自动在 `/usr/lib/debug/boot/`、`/lib/modules/<rel>{,/build}/` 里找 vmlinux |
| ⭐ `debuginfod-find` | 联网自动拉 debuginfo |
| ⭐ **Rust demangler** | 优先 `llvm-cxxfilt`，退化 `c++filt -i`（v6.6 内核里已有 Rust 代码） |
| 模块符号 | 需要 `<modules path>`，否则打印 `WARNING! Modules path isn't set, but is needed to parse this symbol` |
| 地址格式 | 认 `[<addr>]`（老格式）也认 `func+0x42/0x100`（新格式） |
| `basepath=auto` | 用 `kernel_init` 符号自动推断源码根路径 |

#### kallsyms 的取舍

| 开 | 关 |
|----|----|
| Oops 直接可读、无需额外文件 | 符号表进内核映像（体积 +） |
| `perf` / ftrace 能解析符号 | ⭐ **暴露内核布局，削弱 KASLR** |

> ⭐ 安全敏感场景的三件套：`CONFIG_KALLSYMS_ALL`、`kernel.kptr_restrict`、`kernel.dmesg_restrict`。
> 但要注意：**关掉 kallsyms 会让 Oops 变成一堆裸地址**，排障成本急剧上升。
> HFT 交易机这种"不出网、不接待不可信用户"的场景，**开 kallsyms 的收益远大于风险**。

---

### 七、Oops 与 kdump / pstore / kgdb 的衔接点

| 机制 | 挂钩位置 | 时机 |
|------|---------|------|
| ⭐ **kdump** | `oops_end()` → `crash_kexec(regs)` | Oops 时（`kexec_should_crash()` 判定） |
| kdump（panic 路径） | `panic()` → `__crash_kexec(NULL)` | panic 时（步 7 或步 12） |
| ⭐ **pstore / ramoops** | `oops_exit()` → `kmsg_dump(KMSG_DUMP_OOPS)` | Oops 结束时 |
| pstore（panic 路径） | `panic()` → `kmsg_dump(KMSG_DUMP_PANIC)` | panic 中（步 11） |
| ⭐ **kgdb / kdb** | `panic()` → `kgdb_panic(buf)` | **停其他 CPU 之前**（步 6） |
| kprobe / 其他 notifier | `__die_body()` → `notify_die(DIE_OOPS, ...)` | 打印之后、`oops_end()` 之前 |

> ⭐⭐ **`notify_die(DIE_OOPS)` 返回 `NOTIFY_STOP` 会让 Oops 不杀进程**——
> 这是 kdb 能"接住" Oops 让你现场调试的机制：
> `die()` 里 `if (__die(...)) sig = 0;` → `oops_end()` 里 `if (!signr) return;`

---

### 八、现代内核演进（实测版本断崖）

方法：逐版本拉 `kernel/panic.c` / `arch/x86/mm/fault.c` / `arch/x86/kernel/dumpstack.c` 做特征计数。

| 特性 | 引入版本 | 实测判据 |
|------|---------|---------|
| `CONFIG_KALLSYMS` | v2.5 / v2.6 | Oops 直接可读 |
| `CONFIG_VMAP_STACK`（x86） | v4.9 | 栈溢出换 DF 栈 |
| ⭐ `panic_print` | **v5.0** | v4.20 `panic.c` 0 处 → v5.0 9 处 |
| ⭐ **Oops `BUG:` 行新格式** | **v5.2** | v5.1 仍是 `unable to handle kernel %s at %px` |
| `PANIC_PRINT_ALL_CPU_BT` | v5.16~v5.19 | v5.15 0 处 → v5.19 2 处 |
| KFENCE 抢在 Oops 前 | v5.12 | `kfence_handle_page_fault()` |
| ⭐ `kernel.warn_limit` | **v6.2** | v6.1 0 处 → v6.2 6 处 |
| ⚠️ `kernel.oops_limit` | **从未进 mainline** | v5.19 / v6.0 / v6.6 的 `panic.c` 均为 **0 处** |
| `TAINT_FLAGS_COUNT` = 19 | v6.6 | 逐版本增长（早期只有 ~10 位） |

> ⚠️ **记忆纠错**：我原本以为 `oops_limit` 与 `warn_limit` 是 v5.19 一起加的。
> 实测结果是 **`oops_limit` 从未进 mainline**，`warn_limit` 是 **v6.2** 才有的。
> 网上部分教程提到的 `kernel.oops_limit` sysctl 属于**第三方补丁或文档错误**。

---

### 九、HFT / 嵌入式视角

| 关注点 | 建议 |
|--------|------|
| ⭐ **`panic_on_oops=1`** | 交易机上 Oops 后继续跑 = 拿损坏的内核继续下单。宁可重启切备机 |
| `panic=<秒>` | 配正数让 panic 后自动重启；**负值立即重启**；`0` 会挂死等人工 |
| ⭐ **pstore + ramoops** | 无串口的嵌入式设备抓 Oops 的唯一手段；Oops 走 `KMSG_DUMP_OOPS` |
| `panic_print=0x28` | `0x20`（全量 printk）+ `0x08`（锁信息）—— 串口慢导致丢日志时的组合 |
| ⭐ `softlockup_panic=1` | 配合 `TAINT_SOFTLOCKUP`（`L`）：spinlock 死循环比 Oops 更隐蔽 |
| `oops_all_cpu_backtrace=1` | 多核并发故障时一次拿到所有 CPU 的栈 |
| `pause_on_oops=<秒>` | 多核同时炸时防止 backtrace 交错；**串口日志场景必配** |
| ⭐ 模块符号 | 出 Oops 的往往是自研 `.ko`，**必须保留带 debuginfo 的 `.ko` 副本**，否则 `Code:` 行成了唯一线索 |
| `CONFIG_PREEMPT_RT` | Oops 头会打 `PREEMPT_RT`；RT 内核锁语义不同，排障思路也不同 |
| 复现成本 | 低延迟场景的 bug 常与时间相关，`KFENCE` + `KASAN` 的采样开关能在生产上留一道网 |
| ⭐ `CR2` 反推字段 | 自研驱动的 Oops 里，`CR2` 偏移 + `pahole -C` 是最快的定性手段 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** Oops 信息中最重要的字段是什么？如何从 Oops 定位源码？

<details><summary>答案</summary>

关键字段：1) RIP（出错指令地址）→ addr2line 或 objdump 定位源码行；2) Call Trace（调用栈）→ 定位调用链；3) `Code:`（出错指令前后的十六进制）→ 反汇编。`addr2line -e vmlinux <RIP地址>` 定位源码行。`gdb vmlinux` + `list *(RIP地址)` 查看源码。HFT 驱动 Oops 分析是最基本的内核排障技能。

</details>

<details><summary>按 v6.6 修订/补充</summary>

优先级要重排，v6.6 的 **`CR2` 比 `Code:` 更值得先看**：

| 字段 | 为什么 |
|------|--------|
| ⭐ **CR2** | 触发缺页的线性地址。**`CR2 = 0x10` + `RIP = f+0x42` ⇒ 访问了某结构体偏移 0x10 的成员而指针是 NULL**。用 `pahole -C <type> <mod>.ko` 直接反推字段名，一步定性 |
| `RIP` | 出错指令。`func+0x42/0x100` 的分母 `0x100` 是**函数总长度**，可用于判断偏移是否合理 |
| `#PF: error_code(0x…)` | 区分读/写/取指 + 页不存在/权限违规/保留位。`0x0002` = 写空指针 |
| `Oops: … [#N]` | `N > 1` 说明**之前已 Oops 过**，看第一次的 |
| `Tainted:` | 有 `D` = 已 Oops 过；有 `P`/`O`/`E` = 加载了非主线模块 |
| `Call Trace` | ORC unwinder，**v6.6 不再需要 frame pointer** |
| `Code:` | ⭐ **尖括号 `<xx>` 里那个字节就是出错指令首字节**。无 vmlinux 时的唯一线索 |

现代一键解码（不用手工 addr2line）：

```bash
dmesg | ./scripts/decode_stacktrace.sh vmlinux auto /lib/modules/$(uname -r)
```

补两条原答案没提的：
- `ORIG_RAX` **只在非 -1 时才打印**，那个值就是**系统调用号**（呼应 5.3）
- `DR0`-`DR7` **只在非默认状态才出现**，说明当时挂了硬件断点

</details>

**Q2.** Oops 和 panic 的区别？什么时候 Oops 不会变成 panic？

<details><summary>答案</summary>

Oops = 杀死出错进程/线程，系统可能继续运行（如果损坏不严重）。panic = 系统停止，不可恢复。Oops → panic 的条件：1) 在中断上下文中出错（无进程可杀）；2) 损坏关键内核数据结构；3) panic_on_oops 设置为 1。生产 HFT 系统通常设 panic_on_oops=1（损坏的内核不安全，宁可重启）。

</details>

<details><summary>按 v6.6 修订/补充</summary>

`oops_end()` 里**实际只有两个显式判定**，原答案的第 2 条（"损坏关键内核数据结构"）在源码里**没有对应分支**：

```c
	if (!signr)
		return;
	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");
	kasan_unpoison_task_stack(current);
	rewind_stack_and_make_dead(signr);
```

所以完整下场是 **四种**：

| 条件 | 结果 |
|------|------|
| `signr == 0`（`notify_die` 返回 `NOTIFY_STOP`，调试器接管） | **不杀进程，继续跑** |
| `in_interrupt()` | `panic("Fatal exception in interrupt")` |
| `panic_on_oops` 非 0 | `panic("Fatal exception")` |
| 都不成立 | ⭐ `rewind_stack_and_make_dead(signr)` —— **不是普通 `do_exit()`**，要先回卷栈（可能站在 IST 栈上） |

"损坏关键数据结构"之所以常常导致 panic，是因为**后续会再次 Oops 并命中上面两条之一**，或干脆 hard lockup——属于**间接后果**，不是直接判定。

另外：
- 命令行写法只有 `oops=panic` 一个值生效（`oops_setup()` 里 `!strcmp(s, "panic")`）
- `panic_on_oops` 原型是 `int` 但**没有 `oops=N` 计数语义**；计数只在 `WARN()` 侧有 `kernel.warn_limit`（v6.2+）
- 页表损坏走另一条路：`pgtable_bad()` 直接 `__die("Bad pagetable")` + `SIGKILL`

</details>

**Q3.** 内核态踩了坏地址，一定会 Oops 吗？

<details><summary>答案</summary>

**不会。** v6.6 `kernelmode_fixup_or_oops()` 里 Oops 之前有三道拦截：

1. ⭐ **`fixup_exception()`** —— 查 exception table（`__ex_table` 段）。命中就改 `regs->ip` 跳到修复标号，**完全不 Oops**。这正是 `copy_from_user()` / `get_user()` 能安全碰用户指针、返回 `-EFAULT` 而不是崩溃的全部基础
2. **`is_prefetch()`** —— 出错指令是 `PREFETCH` 时直接忽略，注释：*"AMD erratum #91 manifests as a spurious page fault on a PREFETCH instruction"*
3. **`page_fault_oops()`** —— 前两条都不命中才 Oops

`page_fault_oops()` 内部还有两道：
- **KFENCE**（v5.12+）：`kfence_handle_page_fault()` 命中就 `return`，把 use-after-free / 越界变成结构化报告
- **EFI**：`efi_crash_gracefully_on_page_fault(address)` 让坏固件优雅降级

> ⭐ 推论：如果你在驱动里看到"访问了明显非法的地址却没 Oops"，先怀疑是不是**命中了 ex_table 修复路径**——那通常意味着有个 `copy_*_user()` 的调用约定被用错了。

</details>

**Q4.** 内核栈已经踩穿了，Oops 为什么还能打印出 backtrace？

<details><summary>答案</summary>

因为 `page_fault_oops()` 在 Oops 之前**先换了栈**：

```c
	if (is_vmalloc_addr((void *)address) &&
	    get_stack_guard_info((void *)address, &info)) {
		call_on_stack(__this_cpu_ist_top_va(DF) - sizeof(void*),
			      handle_stack_overflow, ASM_CALL_ARG3, ...);
		unreachable();
	}
```

- `CONFIG_VMAP_STACK` 让内核栈在 vmalloc 区，并带 **guard page**
- 踩到 guard page 时 `get_stack_guard_info()` 命中
- ⭐ 用 `call_on_stack()` 把执行切到 **Double-Fault 的 IST 栈**（`__this_cpu_ist_top_va(DF)`）再打印

源码注释解释得极清楚：

> *"We don't want to make it all the way into the oops code and then double-fault, though,
> because we're likely to break the console driver and lose most of the stack dump."*

即：**不换栈的话，打印到一半就 double fault，反而什么都留不下**。

</details>

**Q5.** `Code:` 行那一串十六进制怎么读？为什么是 42 + 1 + 21？

<details><summary>答案</summary>

v6.6 `show_opcodes()`：

```c
#define PROLOGUE_SIZE 42
#define EPILOGUE_SIZE 21
	printk("%sCode: %42ph <%02x> %21ph\n", ..., opcodes,
	       opcodes[PROLOGUE_SIZE], opcodes + PROLOGUE_SIZE + 1);
```

⭐⭐ **尖括号包住的那一个字节，就是 RIP 指向的出错指令首字节。**

42/21 这个怪比例的理由（源码注释，courtesy of Linus）：

| 理由 | 说明 |
|------|------|
| 跨 toolchain 比对 | 没有精确 vmlinux 时，更长前缀便于与不同编译器产出的代码对齐 |
| 重建寄存器分配 | 更多上下文帮助理解寄存器状态 |
| ⭐ **变长指令同步** | x86 是变长指令架构，需要足够长的前缀让反汇编器 **"sync up properly and find instruction boundaries"** |
| 坦白 | *"just a random guesstimate"* |

实操（无 vmlinux 时）：

```bash
# 把 Code: 行字节（去掉尖括号）存成二进制再反汇编
objdump -D -b binary -m i386:x86-64 code.bin
```

然后找到 `<xx>` 那一字节对应的指令即可。

</details>

**Q6.** Oops 里 `Tainted: G` 是什么意思？`P` 呢？

<details><summary>答案</summary>

⭐ **`G` 恰恰表示「没有加载专有模块」**——它是 `taint_flags[TAINT_PROPRIETARY_MODULE]` 的 **`c_false`**：

```c
	[ TAINT_PROPRIETARY_MODULE ]	= { 'P', 'G', true },
	                                  /* ↑c_true ↑c_false */
```

- 置位 → 打印 **`P`**（加载了专有 / 非 GPL 模块）
- 未置位 → 打印 **`G`**（全是 GPL 模块，干净）

⭐ 另一个关键点：**输出是 19 个固定槽位，不是紧凑拼接**：

```c
	for (i = 0; i < TAINT_FLAGS_COUNT; i++) {
		const struct taint_flag *t = &taint_flags[i];
		*s++ = test_bit(i, &tainted_mask) ? t->c_true : t->c_false;
	}
```

所以 `Tainted:` 后面那串字符的**位置本身就编码了含义**（位置 0 = 专有模块，位置 7 = 已 Oops 过，位置 9 = 触发过 WARN……），未置位打空格。全干净时输出 `Not tainted`。

⚠️ 常见误读：把 `Tainted: G` 当成"有专有模块"。**反了。**

</details>

**Q7.** 想让机器"Oops 若干次后再 panic"，能配 `oops=3` 吗？

<details><summary>答案</summary>

**不能。** 实测 v6.6 `kernel/panic.c`：

```c
static int __init oops_setup(char *s)
{
	if (!s)
		return -EINVAL;
	if (!strcmp(s, "panic"))
		panic_on_oops = 1;
	return 0;
}
early_param("oops", oops_setup);
```

- `panic_on_oops` 原型是 `int`（不是 bool）
- 但 **`oops_setup()` 只认 `"panic"` 一个字符串**，且只设成 `1`
- ⚠️ `kernel.oops_limit` sysctl **从未进 mainline**（v5.19 / v6.0 / v6.6 的 `panic.c` 里均为 0 处匹配）

真正存在的计数机制只有 **`kernel.warn_limit`**（**v6.2** 引入），而且作用于 **`WARN()` 不是 Oops**：

```c
void check_panic_on_warn(const char *origin)
{
	if (panic_on_warn)
		panic("%s: panic_on_warn set ...\n", origin);

	limit = READ_ONCE(warn_limit);
	if (atomic_inc_return(&warn_count) >= limit && limit)
		panic("%s: system warned too often (kernel.warn_limit is %d)", origin, limit);
}
```

> 想要"第 N 次异常才 panic"的效果，得自己挂 `panic_notifier_list` 或用 `panic_on_taint`。

</details>

**Q8.** Oops 末尾为什么要再打一遍寄存器？

<details><summary>答案</summary>

两处"executive summary"设计，都是为了防止 **Oops 正文被刷屏冲掉**：

1. `__die_header()` 里**只存第一次** Oops 的寄存器：
```c
	/* Save the regs of the first oops for the executive summary later. */
	if (!die_counter)
		exec_summary_regs = *regs;
```

2. `oops_end()` 末尾再打一遍：
```c
	/* Executive summary in case the oops scrolled away */
	__show_regs(&exec_summary_regs, SHOW_REGS_ALL, KERN_DEFAULT);
```

3. `page_fault_oops()` 里还单独补了一行 CR2：
```c
	/* Executive summary in case the body of the oops scrolled away */
	printk(KERN_DEFAULT "CR2: %016lx\n", address);
```

⭐ 实践意义：
- **只存第一次**——因为第一次才是根因，后续都是连锁
- 串口日志被大量其它 printk 冲掉时，**末尾这份精简版是最后防线**
- 排查"连锁 Oops"时，**直接翻到末尾看 exec summary**，比在正文里找 `[#1]` 快

</details>

**Q9.** `crash_kexec_post_notifiers` 该不该开？

<details><summary>答案</summary>

两难，源码注释给了明确取舍：

| 值 | 顺序 | 优点 | 风险 |
|----|------|------|------|
| **0**（默认） | `__crash_kexec` → 停 CPU → notifiers → kmsg_dump | **尽早抓 vmcore，最可靠** | panic notifier 收集不到信息 |
| **1** | 停 CPU → notifiers → kmsg_dump → `__crash_kexec` | 能先拿到 dmesg / notifier 信息 | ⭐ notifier 可能让崩溃内核更不稳，**kdump 失败风险上升** |

源码原话：

> *"since some panic_notifiers can make crashed kernel more unstable,
> it can increase risks of the kdump failure too."*

**建议**：
- 默认 **0**，除非你确实依赖某个 panic notifier（比如自定义的上报 / 掉电保存）
- 如果主要是想留 dmesg，**优先配 pstore/ramoops**（走 `kmsg_dump(KMSG_DUMP_PANIC)`），而不是开这个开关
- HFT 场景：vmcore 的价值通常高于 notifier 信息，**保持 0**

</details>

**Q10.** 怎么用 `CR2` 从 Oops 直接定位到结构体字段？

<details><summary>答案</summary>

思路：`CR2` = 触发缺页的线性地址。空指针解引用时，它等于 **结构体内的偏移**。

```
CR2: 0000000000000010
RIP: 0010:my_drv_ioctl+0x42/0x100 [mydrv]
```

⇒ 在 `my_drv_ioctl` 里访问了某结构体**偏移 0x10** 的成员，而该结构体指针是 NULL。

三种反推手段：

```bash
# 1) pahole（最方便，来自 dwarves 包）
pahole -C my_dev mydrv.ko

# 2) gdb（有 debuginfo 时）
gdb mydrv.ko
(gdb) p &((struct my_dev *)0)->field_name
(gdb) ptype /o struct my_dev        # 带偏移打印

# 3) crash（有 vmcore 时）
crash> struct my_dev -o
```

然后倒查：**哪条代码路径会把这个指针留成 NULL？**

| 典型根因 | 特征 |
|---------|------|
| 分配失败未检查 | `kmalloc` 返回 NULL 直接用了 |
| `container_of` 用错 | 偏移正好落在结构体后段 |
| 生命周期问题 | 对象已释放（**KFENCE/KASAN 能抓到这类**） |
| 并发竞态 | 另一 CPU 置空了（看 `panic_print=0x40` 拿其他 CPU 栈） |

> ⭐ 这个技巧对**自研驱动**最有效——内核主线的结构体你还得查头文件，
> 自研的 `.ko` 只要保留一份带 debuginfo 的副本，`pahole` 一跑就知道是哪个字段。

</details>

**Q11.** 八个 CPU 同时 Oops，日志会变成什么样子？

<details><summary>答案</summary>

不处理的话：**八个 backtrace 交错输出，什么也看不清**。

v6.6 用 `pause_on_oops` 串行化。核心是全局标志 `pause_on_oops_flag`：

```c
	if (pause_on_oops_flag == 0) {
		/* This CPU may now print the oops message */
		pause_on_oops_flag = 1;
	} else {
		/* We need to stall this CPU */
		...
	}
```

配合拦截点：

```c
bool oops_may_print(void) { return pause_on_oops_flag == 0; }
```

`show_fault_oops()` 第一行就是：

```c
	if (!oops_may_print())
		return;
```

效果：

| 角色 | 行为 |
|------|------|
| 第一个 Oops 的 CPU | 打印完整 Oops |
| 其他 CPU | 在 `do_oops_enter_exit()` 里**空转等待** `pause_on_oops` 秒 |

配置：`pause_on_oops=<秒数>`（内核参数）。

> ⭐ 源码还有一句诚实吐槽：*"the CPU which is allowed to print ends up pausing for the right duration,
> whereas all the other CPUs pause for **twice as long**: once in `oops_enter()`, once in `oops_exit()`."*
> —— 因为 `do_oops_enter_exit()` 被 enter 和 exit 各调一次。
>
> 串口日志 / 慢控制台场景**必配**；默认值 0 表示不串行化。

</details>

**Q12.** `panic_cpu` 这个全局变量是干什么的？

<details><summary>答案</summary>

**保证 panic 主体只被一个 CPU 执行一次。**

```c
	this_cpu = raw_smp_processor_id();
	old_cpu  = atomic_cmpxchg(&panic_cpu, PANIC_CPU_INVALID, this_cpu);

	if (old_cpu != PANIC_CPU_INVALID && old_cpu != this_cpu)
		panic_smp_self_stop();
```

语义：

| `old_cpu` | 含义 | 动作 |
|-----------|------|------|
| `PANIC_CPU_INVALID` (-1) | 第一个进来的 | 继续执行 panic 主体 |
| `== this_cpu` | 从 `nmi_panic()` 进来（它已把 `panic_cpu` 设成本 CPU） | 也视为第一个，继续 |
| 其他 CPU 号 | 已经有别的 CPU 在 panic | ⭐ `panic_smp_self_stop()` —— 停自己 |

注释原话：

> *"Only one CPU is allowed to execute the panic code from here. For multiple parallel
> invocations of panic, all other CPUs either stop themself or will wait until they are
> stopped by the 1st CPU with `smp_send_stop()`."*

配套的第 1 步也很关键：

```c
	/*
	 * Disable local interrupts. This will prevent panic_smp_self_stop
	 * from deadlocking the first cpu that invokes the panic, since
	 * there is nothing to prevent an interrupt handler (that runs
	 * after setting panic_cpu) from invoking panic() again.
	 */
	local_irq_disable();
	preempt_disable_notrace();
```

> ⭐ 注意顺序：**先关中断，再 cmpxchg `panic_cpu`**。
> 否则可能出现"设了 `panic_cpu` 之后被中断打断，中断处理里又 `panic()`"的死锁。

</details>

</details>
---
