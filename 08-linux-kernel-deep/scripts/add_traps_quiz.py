#!/usr/bin/env python3
"""08-linux-kernel-deep 🔴 章节新手化增强脚本
为 8 个🔴章节共 49 篇笔记添加 常见陷阱(3) + 折叠自测题(3-4)
"""
import os, re

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # 08-linux-kernel-deep

def make_block(traps, quiz):
    """生成陷阱+自测块"""
    lines = [
        "### 常见陷阱",
        "",
    ]
    for i, t in enumerate(traps, 1):
        lines.append(f"{i}. {t}")
    lines += [
        "",
        "---",
        "",
        "<details>",
        "<summary>自测题（点击展开）</summary>",
        "",
    ]
    for i, (q, a) in enumerate(quiz, 1):
        lines.append(f"**Q{i}.** {q}")
        lines.append("")
        lines.append(f"<details><summary>答案</summary>")
        lines.append("")
        lines.append(a)
        lines.append("")
        lines.append("</details>")
        lines.append("")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)

def insert_before_nav(content, block):
    """在导航行前的最后 --- 前插入块"""
    marker = "\n---\n\n<--"
    # Actually the nav line starts with ← (U+2190), not <--
    # Let me use a different marker
    lines = content.split("\n")
    nav_idx = None
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].startswith("\u2190"):  # ←
            nav_idx = i
            break
    if nav_idx is None:
        return content.rstrip() + "\n\n" + block + "\n"
    # Find the --- before nav_idx
    sep_idx = nav_idx - 1
    while sep_idx >= 0 and lines[sep_idx].strip() == "":
        sep_idx -= 1
    if sep_idx >= 0 and lines[sep_idx].strip() == "---":
        # Insert before the --- line
        lines.insert(sep_idx, block)
    else:
        # Insert before the nav line
        lines.insert(nav_idx, block)
    return "\n".join(lines)

# ============================================================
# Ch2 Memory Addressing (6 notes)
# ============================================================
CH2 = "chapter-02-memory-addressing/notes"
CH2_NOTES = {
    f"{CH2}/section-1-本章定位.md": (
        [
            "把 ULK Ch2 当成现代 6.x 的权威——它基于 2.6，Linux 现在用五级页表（P4D），ULK 只讲到四级",
            "混淆「分段」和「分页」——x86-64 上分段基本被禁用（flat model），Linux 2.6.20 起就 `__KERNEL_DS = __USER_DS`，分段机制只在 32 位有实际意义",
            "以为 TLB 刷新是全量的——现代内核用 `flush_tlb_mm_range()` 做范围刷新，且支持 PCID (Process Context ID) 避免刷 TLB",
        ],
        [
            ("ULK Ch2 讲的四级页表（PGD→PMD→PTE）在现代 x86-64 上还够用吗？", "不够。6.x 内核在 x86-64 上用五级页表：PGD→P4D→PUD→PMD→PTE。P4D 层是 4.11 引入的，为了支持 57 位虚拟地址（LA57）。ULK 只讲到 PGD→PMD→PTE 三级或 PGD→PUD→PMD→PTE 四级。"),
            ("Linux 64 位上分段机制还有实际作用吗？", "基本没有。x86-64 强制 flat segment model，`CS/DS/SS` 的 base 都是 0，limit 都是全空间。Linux 内核中 `__KERNEL_CS` 和 `__USER_CS` 的 base/limit 相同，区别只在 DPL（权限级）。分段在 32 位时代有意义，64 位已被分页完全取代。"),
            ("HFT 场景下，TLB 刷新为什么是性能杀手？", "TLB miss 会触发硬件 page table walk（4-5 次内存访问）。HFT 热路径应避免 `mmap`/`mprotect` 操作（会触发 TLB shootdown），用大页（2MB/1GB）减少 TLB 条目数，绑核避免 context switch 导致的 TLB 刷新。"),
        ],
    ),
    f"{CH2}/section-2-三种内存地址.md": (
        [
            "把「逻辑地址」和「虚拟地址」混为一谈——逻辑地址是段机制时代的概念（段选择符:偏移），虚拟地址是分段后的线性地址",
            "以为 `__pa()` 宏在所有架构上都一样——x86 上是简单的减去 `PAGE_OFFSET`，ARM64 上可能需要查表",
            "在内核模块中直接用物理地址——内核 API 用的是虚拟地址（`__va()`/`__pa()` 转换），直接操作物理地址会 panic",
        ],
        [
            ("逻辑地址、线性地址（虚拟地址）、物理地址三者的转换链路是什么？", "逻辑地址 →（分段）→ 线性地址/虚拟地址 →（分页）→ 物理地址。在 x86-64 flat model 下，逻辑地址 == 线性地址（段 base=0），所以实际只有一级转换：虚拟地址 →（页表）→ 物理地址。"),
            ("`__pa(x)` 和 `virt_to_phys(x)` 有什么区别？", "`__pa()` 是 x86 架构特定的宏，直接做算术减法（`x - PAGE_OFFSET`）。`virt_to_phys()` 是通用 API，内部调用架构相关的实现。在驱动代码中应始终用 `virt_to_phys()` 而非 `__pa()`，保证可移植性。"),
            ("为什么 HFT 代码要避免在热路径上做 `virt_to_phys()` 转换？", "`virt_to_phys()` 本身开销很小（算术运算），但它暗示你在用直接映射区（direct map）的地址。如果涉及 vmalloc 区或模块区地址，转换需要查页表，开销大。更重要的是，频繁的地址转换说明代码在设计上没有区分好物理连续和虚拟连续的内存使用。"),
        ],
    ),
    f"{CH2}/section-3-分段机制.md": (
        [
            "在 64 位代码中还在纠结 GDT/LDT 的段选择符——x86-64 分段已被废弃，GDT 仍存在但只为权限切换服务",
            "以为 `set_fs()` 还在现代内核中——`set_fs()`/`get_fs()` 在 5.10 被移除，内核/用户态地址检查改用 `access_ok()` + `get_user()`/`put_user()`",
            "混淆 CPL/DPL/RPL——CPL 是当前代码权限（CS 低 2 位），DPL 是段描述符要求的权限，RPL 是选择符中的请求权限",
        ],
        [
            ("GDT 在 x86-64 内核中还有用吗？", "有，但作用大为缩减。GDT 仍用于存放 TSS（任务状态段）、内核栈指针、权限级标记（DPL 0/3）。但段基址/段限长已无意义（flat model）。`syscall` 指令直接从 MSRs 加载 CS/SS，不走 GDT 查表。"),
            ("`set_fs(KERNEL_DS)` 为什么被移除？移除后怎么替代？", "`set_fs()` 临时切换地址限制让内核能直接读写用户空间指针，容易引发安全漏洞（覆盖后忘记恢复）。5.10 起移除，改用显式的 `copy_from_user()`/`copy_to_user()` 和 `get_user()`/`put_user()`，所有用户指针必须通过这些安全函数访问。"),
            ("ULK 讲的段描述符 8 字节结构在 x86-64 上变了吗？", "变了。x86-64 段描述符仍 8 字节，但 64 位代码段描述符（L=1）格式不同，且 64 位 TSS 描述符占 16 字节（两个 GDT slot）。ULK 基于的 32 位描述符格式不能直接用于 64 位分析。"),
        ],
    ),
    f"{CH2}/section-4-硬件分页.md": (
        [
            "把 32 位的两级页表直接套用到 64 位——x86-64 用 4 级或 5 级页表，每级 9 位索引，页表项 8 字节",
            "以为 PTE 中只有物理页帧号——PTE 还包含权限位（R/W, U/S）、状态位（Dirty, Accessed）、缓存属性位（PAT, PCD, PWT）等",
            "混淆 4KB 页和 2MB/1GB 大页的页表层级——大页在 PMD 或 PUD 层就终止 walk，不需要 PTE 层",
        ],
        [
            ("x86-64 四级页表中，虚拟地址 48 位如何分配？", "48 位 = 4×9（页表索引）+ 12（页内偏移）。PGD 9 位 + PUD 9 位 + PMD 9 位 + PTE 9 位 + offset 12 位 = 48 位。每级页表 512 个条目（2^9），每个条目 8 字节，一张页表恰好 4KB（一个页）。"),
            ("PTE 的 Present 位 = 0 时，内核怎么知道是「换出到 swap」还是「从未分配」？", "PTE 不存在（全 0）= 从未映射。PTE Present=0 但非零 = 已换出或文件映射未加载，高 24 位（swap entry）编码了 swap 类型和 offset。内核用 `pte_present()` 判断，`pte_to_swp_entry()` 解码 swap 信息。"),
            ("HFT 为什么推荐用 2MB 大页（huge page）？", "2MB 大页在 PMD 层终止 page walk，省去 PTE 层的一次内存访问。TLB 中一个 2MB 条目覆盖 512 个 4KB 页，大幅减少 TLB miss。对 HFT 热路径（如订单簿内存），`madvise(MADV_HUGEPAGE)` 或 `mmap(MAP_HUGETLB)` 能显著降低延迟抖动。"),
        ],
    ),
    f"{CH2}/section-5-Linux四级分页.md": (
        [
            "以为现代 x86-64 仍用四级页表——5.x 内核已支持五级页表（PGD→P4D→PUD→PMD→PTE），需 CONFIG_X86_5LEVEL=y",
            "把 `pgd_offset()` 的参数搞反——`pgd_offset(mm, address)` 第一个参数是 `mm_struct`，不是 `task_struct`",
            "以为 `pgd_none()` 返回 true 就代表这段地址没被映射——也可能是被 `PROT_NONE` 保护的页，需要看 `pmd_present()` 进一步判断",
        ],
        [
            ("Linux 四级页表和五级页表在代码层面有什么区别？", "五级页表多了一层 P4D。内核用 `pgtable-nopud.h`/`pgtable-nop4d.h` 等头文件在编译时折叠不存在的层级，使四级硬件在软件层面仍呈现五级接口。`CONFIG_X86_5LEVEL=y` 时 P4D 真实存在于硬件页表中。"),
            ("`pgd_offset(mm, addr)` 返回什么？怎么进一步拿到 PTE？", "返回 `pgd_t*` 指针。链路：`pgd_offset(mm, addr)` → `p4d_offset(pgd, addr)` → `pud_offset(p4d, addr)` → `pmd_offset(pud, addr)` → `pte_offset_map(pmd, addr)`。每一步都要检查 `*_none()` 或 `*_present()`。"),
            ("为什么内核要把三级/四级/五级页表统一成五级软件接口？", "可移植性。不同架构页表级数不同（ARM64 可配 3/4/5 级，x86-64 可配 4/5 级）。统一成五级接口后，通用代码不用 `#ifdef` 区分级数——不存在的层会被折叠成「transparent」操作（直接传递指针）。"),
        ],
    ),
    f"{CH2}/section-6-内存布局与TLB.md": (
        [
            "以为内核虚拟地址空间布局和 ULK 讲的一样——6.x 内核的布局有变化（module 区移到 0xffffffffa0000000 附近，KASLR 打乱了内核代码基址）",
            "混淆 `ZONE_DMA`/`ZONE_DMA32`/`ZONE_NORMAL` 的边界——这取决于架构，x86-64 上 DMA=16MB，DMA32=4GB，其余是 NORMAL",
            "以为 TLB 刷新是即时的——TLB shootdown 需要跨 CPU IPI，是异步操作，在 HFT 场景可能导致微秒级抖动",
        ],
        [
            ("x86-64 内核虚拟地址空间的三大区域是什么？", "① direct mapping（直射区）：`0xffff888000000000` 起，映射所有物理内存；② vmalloc area：`0xffffc90000000000` 起，用于 `vmalloc()`；③ vmemmap：`0xffffea0000000000` 起，`struct page` 数组。ULK 讲的布局基于 32 位，地址完全不同。"),
            ("为什么 HFT 要避免跨 CPU 的内存操作？", "跨 CPU 访问共享数据可能导致 TLB shootdown（IPI 中断其他 CPU 刷新 TLB），耗时数微秒。绑核 + per-CPU 数据结构可以避免这个问题。`/proc/interrupts` 中的 `TLB` 行可以观察 shootdown 频率。"),
            ("KASLR 对内核调试有什么影响？", "KASLR（Kernel Address Space Layout Randomization）随机化内核代码加载基址，`/proc/kallsyms` 默认显示 0 地址（非 root）。调试时需要 `nokaslr` 启动参数禁用，或用 `kptr_restrict=0` 暴露真实地址。HFT 生产环境通常保留 KASLR（安全）但调试时禁用。"),
        ],
    ),
}

# ============================================================
# Ch3 Processes (6 notes)
# ============================================================
CH3 = "chapter-03-processes/notes"
CH3_NOTES = {
    f"{CH3}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 `task_struct` 字段当现代版——6.x 的 `task_struct` 已超过 8000 字节，字段布局和 ULK 时代完全不同",
            "混淆「线程」和「进程」在内核层面的区别——内核不区分，都是 `task_struct`，线程是共享地址空间的 `task_struct`",
            "以为 `current` 宏在所有架构上一样——x86 用 `per_cpu` 变量，ARM64 用 `sp_el0` 寄存器存储",
        ],
        [
            ("内核如何区分「进程」和「线程」？", "不区分。每个 `task_struct` 都是一个「内核可调度实体」。进程 = 独立地址空间的 `task_struct`；线程 = 共享 `mm_struct` 的 `task_struct`。`clone(CLONE_VM | CLONE_FILES | CLONE_SIGHAND, ...)` 创建线程，`clone(SIGCHLD, ...)` 创建进程。`task_struct->mm` 指向共享的 `mm_struct`，内核线程 `mm` 为 NULL。"),
            ("`current` 宏在 x86-64 和 ARM64 上实现有何不同？", "x86-64：`current` 从 per-CPU 变量 `current_task` 读取，通过 `gs` 段寄存器基址偏移访问。ARM64：`current` 存在 `sp_el0` 寄存器中（内核态 SP_EL0 存 `task_struct` 指针），直接 `mrs x0, sp_el0` 读取。ARM64 方式更快（零内存访问），但 `sp_el0` 在内核态被复用。"),
            ("HFT 为什么要把交易线程和内核线程分开？", "内核线程（kworker/softirq）可能抢占交易线程的 CPU。HFT 做法：① 交易线程绑独立核（`sched_setaffinity`）+ `SCHED_FIFO` 实时优先级；② `isolcpus` 隔离该核不让普通任务调度；③ 中断重定向到其他核（`/proc/irq/*/smp_affinity`）。"),
        ],
    ),
    f"{CH3}/section-2-进程与线程.md": (
        [
            "以为 `fork()` 会复制整个地址空间——实际用 COW（Copy-On-Write），只复制页表，物理页共享直到写操作",
            "混淆 `clone()` 的 flag 组合——`CLONE_VM` 共享内存，`CLONE_FILES` 共享文件描述符表，`CLONE_THREAD` 加入同一线程组",
            "以为内核线程和用户线程的创建方式相同——内核线程用 `kthread_create()`/`kthread_run()`，不走 `clone()` 系统调用",
        ],
        [
            ("`fork()` 后子进程的 `task_struct` 哪些字段会变？哪些不变？", "变：PID、PPID（=父 PID）、信号_pending 清空、`mm` 的引用计数+1、页表 COW 复制。不变：`mm` 指针（共享但 COW）、`files`（共享但引用计数+1）、`fs`（共享 CWD/root）、调度策略/nice。`fork()` 返回值：父进程=子 PID，子进程=0。"),
            ("`clone(CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD)` 创建的是什么？", "线程。`CLONE_VM` 共享地址空间，`CLONE_FILES` 共享 fd 表，`CLONE_SIGHAND` 共享信号处理，`CLONE_THREAD` 放入同一线程组（`tgid` 相同，`pid` 不同）。这就是 `pthread_create()` 底层的 `clone()` 调用。"),
            ("内核线程为什么 `mm` 为 NULL？它怎么访问内核内存？", "内核线程不拥有用户地址空间，`task_struct->mm = NULL`。它通过 `active_mm`（借用的 `mm_struct`）访问内核态地址（内核地址空间在所有 `mm_struct` 中都相同）。`active_mm` 在 schedule 时被设置为前一个用户进程的 `mm`，避免 TLB 刷新。"),
        ],
    ),
    f"{CH3}/section-3-进程描述符.md": (
        [
            "把 ULK 的 `task_struct` 字段布局当现代版——6.x 新增了大量字段（cgroup、seccomp、io_uring、KVM 等），旧字段也有移除",
            "以为 PID 就是 `task_struct->pid`——线程的 `pid` 是线程 ID，`tgid`（线程组 ID）才是用户态看到的「进程 PID」",
            "混淆 `task_struct` 的双向链表和运行队列——`tasks` 链表遍历所有进程，运行队列是 `cfs_rq`/`rt_rq`",
        ],
        [
            ("用户态 `getpid()` 返回的是 `task_struct->pid` 还是 `->tgid`？", "返回 `tgid`。内核中 `task_struct->pid` 是唯一的线程 ID，`->tgid` 是线程组 leader 的 PID。用户态的 `getpid()` 实际调 `sys_getpid()` 返回 `current->tgid`。`gettid()` 才返回 `current->pid`。单线程进程 leader 中 `pid == tgid`。"),
            ("`task_struct` 中 `tasks` 链表和 `children` 链表有什么区别？", "`tasks`：全局所有 `task_struct` 的双向链表（`init_task` 为头），用于遍历系统所有进程。`children`：当前进程的子进程链表（`init_task` 的 children 是所有孤儿进程的祖先链）。`/proc/[pid]/task/` 遍历的是同一线程组的 `thread_node` 链表。"),
            ("为什么 `task_struct` 不能放在内核栈底部？", "历史原因 + 安全。ULK 时代 x86-32 用 `current` = `ESP & ~8191`（栈底即 `task_struct`），快速但浪费——每个进程 8KB 栈中 2KB 给 `thread_info`。x86-64 改用 per-CPU 变量存 `current`，`thread_info` 移到 `task_struct` 内部。好处是栈空间增大（16KB/32KB），坏处是 `current` 访问多一次 per-CPU 偏移。"),
        ],
    ),
    f"{CH3}/section-4-组织与查找.md": (
        [
            "以为 PID 查找还是 ULK 讲的哈希表——6.x 用 `pidfd` 机制和 IDR/XArray 管理 PID 查找",
            "混淆 `find_task_by_pid()` 和 `pid_task()`——前者已不推荐，后者是现代 API",
            "以为 wait queue 只用于 `wait()` 系统调用——wait queue 是通用等待机制，中断/定时器/锁都使用",
        ],
        [
            ("现代内核如何通过 PID 快速查找 `task_struct`？", "PID → `struct pid`（通过 `find_vpid()`，走 namespace 感知的 IDR/XArray 查找）→ `pid_task(pid, PIDTYPE_PID)` → `task_struct`。ULK 讲的 PID 哈希表已被 `pidhash` + IDR 取代。`pidfd_open()` 是 5.x 新增的 race-free PID 管理 API。"),
            ("wait queue 和 completion 有什么区别？什么时候用哪个？", "wait queue：通用等待机制，支持条件等待（`wait_event()`）、多等待者、自定义条件。completion：专门用于「一次性完成通知」，`wait_for_completion()` + `complete()`，语义简单且无 spurious wakeup。驱动初始化等待硬件就绪用 completion；等待条件变量用 wait queue。"),
            ("HFT 中为什么要避免 `find_task_by_pid()` 在热路径上调用？", "`find_task_by_pid()` 需要 RCU 读锁 + IDR 查找 + namespace 处理，开销在百纳秒级。HFT 热路径应缓存 `task_struct` 指针或 PID fd，避免重复查找。更好的做法是用 `pidfd` 在初始化时获取引用，热路径直接解引用。"),
        ],
    ),
    f"{CH3}/section-5-进程切换.md": (
        [
            "把 ULK 讲的 `switch_to()` 宏当现代版——6.x 的 `switch_to()` 是架构相关内联汇编，且加入了 spectre/meltdown 缓解",
            "以为上下文切换只保存寄存器——还要切换 `mm_struct`（`switch_mm()`）、FPU 状态、TLS 段、信号掩码",
            "混淆 `schedule()` 和 `context_switch()`——`schedule()` 选下一个进程，`context_switch()` 执行切换",
        ],
        [
            ("`context_switch()` 的两个核心步骤是什么？", "① `switch_mm()`（或 `activate_mm()`）：切换 `mm_struct`，加载新页表（`CR3` 寄存器写入），刷新 TLB（如有必要）。② `switch_to()`：保存当前寄存器到 `thread_struct`，恢复新进程寄存器，跳转到新进程的返回点。`switch_mm()` 开销远大于 `switch_to()`（页表切换 + TLB 刷新）。"),
            ("为什么内核线程切换不需要 `switch_mm()`？", "内核线程 `mm = NULL`，不拥有用户地址空间。切换到内核线程时，`active_mm` 保留前一个用户进程的 `mm`，不写 `CR3`，不刷 TLB。这就是「lazy TLB」——内核线程借用前一个进程的地址空间映射，避免昂贵的 TLB 刷新。"),
            ("HFT 中 context switch 的实际开销是多少？如何测量？", "单核切换：~1-3 us（含 `switch_mm`），纯 `switch_to`：~100-300 ns。测量：`perf bench sched messaging` 或 `context_switch` 微基准。HFT 减少切换的方法：① 绑核 + `isolcpus` 消除抢占调度；② `SCHED_FIFO` 避免被普通进程抢占；③ 减少 system call（每次 syscall 返回可能触发调度）。"),
        ],
    ),
    f"{CH3}/section-6-创建与销毁.md": (
        [
            "以为 `fork()` 立即复制内存——COW 机制下只复制页表（PTE 设为只读），物理页延迟到第一次写时才复制",
            "混淆 `exit()` 和 `_exit()`——`exit()` 是 glibc 包装（跑 atexit handler + flush stdio），`_exit()`/`sys_exit_group()` 是内核直接终止",
            "以为僵尸进程是 bug——这是正常状态，父进程还没 `wait()` 回收子进程的退出状态",
        ],
        [
            ("`fork()` 中 COW 的具体流程是什么？", "① `dup_mm()` 复制 `mm_struct` 和页表（PTE），所有 PTE 设为只读。② 物理页不复制，引用计数 +1。③ 子进程写某页 → page fault → `do_wp_page()` → 分配新物理页，复制内容，PTE 改为可写，旧页引用计数 -1。COW 省内存但首次写有 fault 开销。"),
            ("进程退出时内核做了哪些清理？", "① `do_exit()`：释放 `mm_struct`（如引用计数归零）、关闭 fd、释放信号队列、从 PID 哈希/任务链表移除。② 状态设为 `EXIT_ZOMBIE`，保留 `task_struct`（含退出码 `exit_code`）等待父进程 `wait()`。③ 父进程 `wait()` → `release_task()` 释放 `task_struct`。孤儿进程由 `init`（PID 1）自动回收。"),
            ("HFT 中为什么要避免在热路径上 `fork()`/`exec()`？", "`fork()` 复制页表的开销与进程地址空间大小成正比（大程序可达毫秒级）。`exec()` 更昂贵：丢弃页表 + 加载 ELF + 重新初始化地址空间。HFT 进程应在启动时 `fork` + `exec` 所有 worker，之后不再创建新进程。用 `posix_spawn()` 或 `vfork()`（不复制页表）可减小 `fork` 开销。"),
        ],
    ),
}

# ============================================================
# Ch4 Interrupts (8 notes)
# ============================================================
CH4 = "chapter-04-interrupts-and-exceptions/notes"
CH4_NOTES = {
    f"{CH4}/section-1-本章定位.md": (
        [
            "把 ULK 的 IDT 结构直接用于 64 位分析——x86-64 IDT 条目格式不同（16 字节，含 IST 字段），且 `int $0x80` 不再是 syscall 入口",
            "混淆「中断」和「异常」——中断是异步的（硬件触发），异常是同步的（指令执行触发）",
            "以为中断处理不能睡眠——传统中断（hard IRQ）不能睡眠，但 threaded IRQ 和 workqueue 可以",
        ],
        [
            ("ULK Ch4 讲的中断处理框架在现代内核中最大的变化是什么？", "① x86-64 用 `IDTENTRY` 宏统一管理 IDT 条目，取代手写汇编 stub。② `int $0x80` 被 `syscall` 指令取代作为系统调用入口。③ threaded IRQ（`request_threaded_irq()`）允许中断处理函数在内核线程中运行，可以睡眠。④ `irq_desc` 层级从全局数组改为 per-domain 的 IRQ domain 树。"),
            ("hard IRQ 为什么不能睡眠？threaded IRQ 怎么解决这个限制？", "hard IRQ 运行在中断上下文（无 `task_struct`、无可调度实体），调度器无法切换。睡眠需要 `schedule()`，会 panic。threaded IRQ 把中断处理拆成两半：hard IRQ 只确认硬件 + 唤醒内核线程，实际处理在线程中运行（有 `task_struct`，可调度可睡眠）。用 `request_threaded_irq(dev, hard_fn, thread_fn, flags, ...)` 注册。"),
            ("HFT 中如何减少中断对热路径的干扰？", "① `irqbalance` off + 手动绑中断到非交易核（`/proc/irq/[n]/smp_affinity`）。② `napi` 轮询模式代替中断驱动收包。③ DPDK 完全绕过内核中断，用用户态轮询。④ `isolcpus` + `nohz_full` 减少定时器中断。"),
        ],
    ),
    f"{CH4}/section-2-中断与异常分类.md": (
        [
            "混淆 fault/trap/abort——fault 可恢复（缺页），trap 用于调试（int3），abort 不可恢复（double fault）",
            "以为所有异常都有 error code——只有部分异常推送 error code（如 page fault 推送 CR2），`int3`/`overflow` 不推送",
            "把 ULK 的 32 位异常号和 64 位混淆——64 位异常号分配有调整，且增加了 IST（Interrupt Stack Table）机制",
        ],
        [
            ("Fault、Trap、Abort 三类异常的区别和典型例子？", "Fault：可恢复，CPU 恢复到触发指令重新执行（如 #PF 缺页、#GP 段错误）。Trap：调试用，CPU 恢复到下一条指令（如 #DB 断点、`int3`）。Abort：不可恢复，通常 panic（如 #DF double fault、#MC machine check）。HFT 中 #PF 在热路径上是大忌（微秒级延迟尖峰）。"),
            ("x86-64 异常处理相比 32 位有什么新机制？", "① IST（Interrupt Stack Table）：某些关键异常（#DF, #NMI, #MC）切换到专用内核栈，避免栈溢出导致二次异常。② `IDTENTRY` 宏自动处理 error code 和栈切换。③ syscall 指令不走 IDT，直接从 MSR 加载入口。④ 64 位下 #SS（stack segment fault）基本不会触发。"),
            ("HFT 如何检测热路径上的异常（如 page fault）？", "① `perf stat -e page-faults` 统计缺页次数。② `bpftrace -e 'tracepoint:exceptions:page_fault_user { @[comm] = count(); }'` 按进程统计。③ `/proc/[pid]/stat` 的 `minflt`（minor fault）和 `majflt`（major fault）字段。④ HFT 应确保热路径 `minflt = 0`（预分配 + 大页 + mlock）。"),
        ],
    ),
    f"{CH4}/section-3-IDT与门描述符.md": (
        [
            "把 ULK 的 8 字节门描述符用于 64 位——x86-64 IDT 条目是 16 字节，含 IST 字段和新的属性位",
            "以为 `int $0x80` 还是现代 syscall 入口——x86-64 用 `syscall` 指令 + MSR_LSTAR，`int $0x80` 保留但慢且不推荐",
            "混淆中断门和陷阱门——中断门自动关 IF（CLI），陷阱门不关，syscall 指令用专用机制",
        ],
        [
            ("x86-64 IDT 条目和 32 位有什么区别？", "32 位：8 字节，`offset[31:0]` 拆成两段。64 位：16 字节，`offset[63:0]` 拆成三段，新增 IST 字段（3 位，指定专用栈），`type` 字段中中断门=0xE，陷阱门=0xF。64 位 IDT 条目还包含 `CS` 选择符（必须指向 64 位代码段）。ULK 的 8 字节格式不能用于 64 位分析。"),
            ("IST（Interrupt Stack Table）解决什么问题？", "某些异常（double fault, NMI, machine check）在当前栈损坏时无法处理。IST 让这些异常切换到预定义的专用内核栈（TSS 中的 `IST[n]` 指针），保证有干净的栈可用。典型场景：内核栈溢出 → #DF → 用 IST 栈处理 → panic 而非 triple fault。"),
            ("`syscall` 指令相比 `int $0x80` 有什么优势？", "① 不走 IDT 查表，直接从 `MSR_LSTAR` 加载入口地址（更快）。② 不压入 error code/SS/CS 的旧式帧，只存 `RIP` 到 `RCX`、`RFLAGS` 到 `R11`。③ 自动切换 `CS`/`SS` 到内核段（从 MSR 加载）。④ 不修改 IF 标志（中断保持开启）。实测 `syscall` 比 `int $0x80` 快 ~3-5 倍。"),
        ],
    ),
    f"{CH4}/section-4-控制路径嵌套.md": (
        [
            "以为中断可以无限嵌套——现代内核限制了嵌套深度，且 hard IRQ 中不可嵌套同号中断",
            "混淆「中断上下文」和「进程上下文」——中断上下文无 `task_struct`、不可睡眠、不可调度",
            "以为 `local_irq_disable()` 只禁当前 CPU——确实只禁本地 CPU，其他 CPU 仍可收中断",
        ],
        [
            ("内核如何跟踪中断嵌套深度？", "`current->preempt_count` 中有 `HARDIRQ_OFFSET`（8-12 位）和 `SOFTIRQ_OFFSET`（13-16 位）。每进一层 hard IRQ，`preempt_count` += `HARDIRQ_OFFSET`；退出时 -=。`in_irq()` 检查是否在 hard IRQ，`in_softirq()` 检查是否在 softirq。`in_interrupt()` 检查任意中断上下文。"),
            ("`local_irq_disable()` / `local_irq_save()` 的区别？", "`local_irq_disable()` 无条件关中断，不保存之前的状态——如果你不知道调用前中断是否已关，用这个可能破坏调用者的状态。`local_irq_save(flags)` 保存 `RFLAGS.IF` 到 `flags` 再关中断，`local_irq_restore(flags)` 恢复。内核代码应始终用 `_save`/`_restore` 版本。"),
            ("为什么 HFT 在用户态也要关心中断嵌套？", "用户态虽然不直接处理中断，但中断会抢占用户线程的 CPU 执行。一次 NIC 硬中断 → softirq → 其他进程被调度，可能导致交易线程停顿数十微秒。解决方案：① `isolcpus` 隔离交易核，中断路由到其他核。② `preempt=full` + `SCHED_FIFO` 让交易线程不可被抢占。③ DPDK 用户态轮询完全绕过中断。"),
        ],
    ),
    f"{CH4}/section-5-异常处理.md": (
        [
            "以为所有异常都走 `do_page_fault()`——只有 #PF（异常 14）走缺页路径，#GP/#UD 等走各自的 handler",
            "混淆缺页异常的「minor fault」和「major fault」——minor 是 COW/demand paging（内存中解决），major 要读磁盘",
            "在异常处理中做复杂操作——异常处理应尽量简单，复杂逻辑交给上层或下半部",
        ],
        [
            ("page fault handler 的主要判断流程是什么？", "① 读 `CR2` 获取故障地址。② 在 `current->mm` 的 VMA 中查找（现代内核用 maple tree）。③ 找到 VMA → 检查权限（读/写/执行 vs VMA flags）。④ 权限 OK → demand paging 或 COW。⑤ 无 VMA 或权限不符 → `SIGSEGV`。⑥ 内核态 fault → `fixup_exception()` 搜索异常表，找到 fixup 地址则跳转，否则 `die()` / panic。"),
            ("minor fault 和 major fault 在 HFT 中分别意味着什么？", "Minor fault：页在内存但 PTE 不存在（demand paging/COW），处理时间 ~1-5 us。Major fault：页需从磁盘/swap 读入，处理时间 ~毫秒级。HFT 热路径两种都不能有：`mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定内存防 swap，预分配 + `MAP_POPULATE` 预填充页表消除 demand paging。"),
            ("内核态访问用户态指针触发 page fault 时怎么处理？", "`copy_from_user()` 等 API 在异常表中注册了 fixup entry。如果用户指针触发 #PF 且 fault 不可恢复（如无 VMA），`do_page_fault()` → `fixup_exception()` 找到对应 fixup → 跳到 `copy_from_user` 的错误返回点 → 返回 `-EFAULT`。这就是为什么内核用 `copy_from_user()` 而非直接解引用用户指针。"),
        ],
    ),
    f"{CH4}/section-6-IO中断处理.md": (
        [
            "以为 IRQ 号就是硬件中断号——现代用 IRQ domain + 虚拟 IRQ 号（virq），硬件号和 Linux IRQ 号不同",
            "混淆 `request_irq()` 和 `request_threaded_irq()`——前者不能睡眠，后者可在内核线程中处理",
            "以为中断处理函数返回 IRQ_HANDLED 就完了——还要操作硬件 ACK/EOI，否则同一中断不会再触发",
        ],
        [
            ("现代内核的 IRQ domain 机制解决了什么问题？", "ULK 时代 IRQ 号 = 硬件中断号，全局数组 `irq_desc[]` 直接索引。现代内核支持中断控制器级联（GIC → GPIO → MSI），硬件号可能冲突。IRQ domain 为每级中断控制器建立独立的号码空间，通过 `irq_domain_translate()` 将硬件号映射为唯一的 Linux virtual IRQ（virq）。`/proc/interrupts` 显示的是 virq。"),
            ("`request_threaded_irq()` 相比 `request_irq()` 有什么优势？", "允许把中断处理拆成 hard IRQ（确认硬件 + 唤醒）和 thread_fn（实际处理，可睡眠）。优势：① thread_fn 可以做 I/O、分配内存、持 mutex。② 减少 hard IRQ 时间，降低中断延迟。③ RT 内核（PREEMPT_RT）强制所有中断线程化。劣势：增加一次唤醒 + 调度延迟。"),
            ("HFT 中如何确保 NIC 中断不干扰交易核？", "① `ethtool -L eth0 combined 1` 减少中断队列数。② `cat /proc/irq/[n]/smp_affinity_list` 设为非交易核。③ `service irqbalance stop` 禁止自动迁移。④ 如果用内核网络栈，配置 RPS/RFS 把 softirq 也路由到非交易核。⑤ 最佳方案：DPDK 绕过内核，用户态轮询收包。"),
        ],
    ),
    f"{CH4}/section-7-可延迟函数与工作队列.md": (
        [
            "混淆 softirq、tasklet、workqueue——softirq 静态编译、tasklet 基于 softirq、workqueue 基于内核线程可睡眠",
            "以为 tasklet 还在现代内核中被推荐——tasklet 已被标记 deprecated，推荐用 workqueue 或 threaded IRQ 替代",
            "在 softirq 中调用睡眠函数——softirq 上下文不能 `schedule()`/`mutex_lock()`，只能用 `spin_lock()`",
        ],
        [
            ("softirq、tasklet、workqueue 三者的关键区别？", "softirq：编译时静态注册（`DEFINE_PER_CPU`），运行在 softirq 上下文，不可睡眠，性能最高。tasklet：基于 softirq（HI_SOFTIRQ/TASKLET_SOFTIRQ），动态注册，同类型不并发，已 deprecated。workqueue：运行在内核线程（`kworker`），可睡眠/持 mutex/做 I/O，性能最低但最灵活。"),
            ("为什么 tasklet 被 deprecated？推荐用什么替代？", "Tasklet 有设计缺陷：① 同类型 tasklet 全局串行化（不能多 CPU 并发），性能差。② 基于 softirq，不能睡眠。③ API 复杂。推荐替代：需要并发 → workqueue（`alloc_workqueue()` + `queue_work()`）。需要低延迟 → threaded IRQ。需要定时回调 → `hrtimer` + softirq。"),
            ("HFT 中 softirq 对延迟有什么影响？怎么排查？", "NIC 收包走 softirq（`NET_RX_SOFTIRQ`），在 `ksoftirqd` 或中断返回时执行。如果 softirq 积压，收包延迟增大。排查：① `/proc/softirqs` 看 `NET_RX` 计数。② `perf top -e irq:softirq:net_rx` 观察执行频率。③ `cat /proc/[pid]/stat` 的 `delayacct_blkio_ticks`。解决：绑 softirq 到非交易核，或用 NAPI/DPDK 轮询。"),
        ],
    ),
    f"{CH4}/section-8-中断返回.md": (
        [
            "以为中断返回就是简单的 IRET——x86-64 用 `IRET` 但还需要处理 preempt count、need_resched、信号传递",
            "混淆中断返回到用户态和返回到内核态——返回用户态要检查 `TIF_NEED_RESCHED`/信号，返回内核态一般不重新调度",
            "以为 `local_irq_enable()` 立即响应所有 pending 中断——需要先处理 preempt count 再开中断",
        ],
        [
            ("中断返回时内核检查哪些条件？", "① `preempt_count` 归零（所有中断/锁计数退出）。② `need_resched` 标志（`TIF_NEED_RESCHED`）→ 触发 `schedule()`。③ `need_resched` + 返回用户态 → 检查 pending 信号 → `do_signal()`。④ 返回用户态 → 可能需要 `audit`/`seccomp` 检查。⑤ x86 上还要检查 `TIF_NOTIFY_RESUME`（task work）。"),
            ("为什么中断返回到内核态一般不重新调度？", "中断打断的是内核代码，内核代码通常持有锁或处于临界区。如果中断返回时调度到其他进程，可能导致锁持有时间过长或死锁。只有在 `preempt_count == 0`（无锁）且 `need_resched` 时才允许内核态抢占调度。`CONFIG_PREEMPT=y` 启用内核抢占，`CONFIG_PREEMPT_NONE` 禁用（服务器默认）。"),
            ("HFT 如何利用 `nohz_full` 减少定时器中断？", "`nohz_full=N` 标记 N 号 CPU 为 full nohz，该 CPU 上只有一个任务运行时，内核停止周期性定时器中断（tickless）。效果：① 消除每秒 100/250/1000 次的 `scheduler_tick()`。② 减少上下文切换。③ 降低 cache 污染。配置：`nohz_full=2-3 isolcpus=2-3 rcu_nocbs=2-3`。注意：该 CPU 上不能有多个竞争 CPU 的任务。"),
        ],
    ),
}

# ============================================================
# Ch5 Kernel Synchronization (7 notes)
# ============================================================
CH5 = "chapter-05-kernel-synchronization/notes"
CH5_NOTES = {
    f"{CH5}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 BKL（大内核锁）当现代机制——BKL 在 2.6.37 完全移除，现代内核不存在",
            "以为内核同步只需要锁——还需要 memory barrier、原子操作、RCU 等无锁机制",
            "混淆 SMP 和 UP 的同步需求——UP 上自旋锁退化为禁用抢占，但仍需要禁用抢占保护临界区",
        ],
        [
            ("ULK 讲的哪些同步机制在现代内核中已被删除？", "① BKL（Big Kernel Lock，`lock_kernel()`）在 2.6.37 完全移除。② `seqlock` 仍存在但使用场景缩小。③ tasklet 正在被废弃。仍有效的：spinlock、mutex、semaphore、RCU（但版本更新了——Tree RCU、Sleepable RCU）。新增的：`refcount_t`（防溢出）、`percpu_rwsem`、`lockdep`（运行时锁依赖检测）。"),
            ("为什么 UP（单处理器）上仍需要同步机制？", "UP 上没有真正的并行，但有**抢占**——内核代码可能被中断/抢占打断。spinlock 在 UP 上退化为 `preempt_disable()`（防止当前 CPU 被抢占）。但不需要关中断（除非中断也访问该数据）。mutex 在 UP 上只禁用抢占，不做原子操作。"),
            ("HFT 用户态为什么也要关心内核同步？", "用户态代码通过 syscall 进入内核，内核中的锁竞争会直接增加 syscall 延迟。例：多线程频繁 `futex` → 内核 `futex` lock 竞争 → 延迟抖动。解决：① 减少系统调用频率（batching）。② 用无锁数据结构（`std::atomic`）替代 `futex`。③ `isolcpus` 减少内核线程竞争。④ `perf lock` 分析锁竞争。"),
        ],
    ),
    f"{CH5}/section-2-内核抢占.md": (
        [
            "以为内核不可被抢占——`CONFIG_PREEMPT=y` 内核允许在大部分内核代码中抢占（除持锁区域）",
            "混淆 `preempt_disable()` 和 `local_irq_disable()`——前者只防抢占，后者还防中断",
            "在 RT 内核（PREEMPT_RT）上以为 spinlock 还是非抢占的——RT 内核的 spinlock 会变成可睡眠的 rt_spinlock",
        ],
        [
            ("`CONFIG_PREEMPT_NONE`/`VOLUNTARY`/`FULL`/`RT` 四种抢占模型有什么区别？", "NONE：内核中不可抢占（服务器默认，吞吐优先）。VOLUNTARY：在 `might_sleep()` 点自愿让出（桌面）。FULL：除持锁/中断上下文外可抢占（低延迟桌面/嵌入式）。RT：几乎所有内核代码可抢占，spinlock 变成可睡眠 mutex（工业实时）。HFT 通常用 FULL + `isolcpus`，或 RT + `SCHED_FIFO`。"),
            ("`preempt_disable()` 后能调用 `schedule()` 吗？", "不能。`preempt_disable()` 递增 `preempt_count`，`schedule()` 检查 `preempt_count == 0` 才允许调度。违反会触发 `BUG: scheduling while atomic` panic。如果需要在不可抢占区域让出 CPU，用 `preempt_enable_no_resched()` + `schedule()` + `preempt_disable()` 手动管理。"),
            ("PREEMPT_RT 内核对 HFT 有什么影响？", "RT 内核把 spinlock 改成可睡眠的 `rt_spinlock`（基于 mutex），中断线程化，`local_irq_disable()` 用 `migrate_disable()` 替代。好处：确定性延迟（最大抢占延迟有界）。坏处：① spinlock 开销增大（从 ~20ns 到 ~200ns）。② 吞吐下降 ~10-30%。HFT 通常评估后选择 FULL（非 RT）+ 手动优化，而非直接用 RT。"),
        ],
    ),
    f"{CH5}/section-3-基础同步原语.md": (
        [
            "把原子操作当万能锁——原子操作只保证单个操作原子性，不保证多操作组合的原子性",
            "混淆 `atomic_t` 和 `refcount_t`——refcount_t 防溢出（不会从 0 下溢到 -1），atomic_t 会",
            "以为 `smp_mb()` 在所有架构上一样——x86 有较强的内存模型，很多 barrier 是空操作；ARM64 需要真正的 barrier 指令",
        ],
        [
            ("`atomic_t` 和 `refcount_t` 的区别？为什么推荐用 `refcount_t`？", "`atomic_t`：纯原子计数器，无防溢出。`atomic_dec(&v)` 可以从 0 变成 -1（UAF 漏洞）。`refcount_t`：引用计数专用，`refcount_dec()` 在 0 时 WARN + 阻止下溢。6.x 内核中 `task_struct` 的 `usage` 已从 `atomic_t` 改为 `refcount_t`。安全代码应始终用 `refcount_t` 管理生命周期。"),
            ("`smp_mb()` / `smp_rmb()` / `smp_wmb()` 分别保证什么？", "`smp_mb()`：全屏障，之前的读写和之后的读写都不可重排。`smp_rmb()`：读屏障，之前的读不可重排到之后的读之后。`smp_wmb()`：写屏障，之前的写不可重排到之后的写之后。x86 上 `smp_rmb()` 是空操作（loads 不重排），`smp_wmb()` 也是空操作（stores 不重排），只有 `smp_mb()` 有 `mfence`。ARM64 上三者都是真实指令。"),
            ("HFT 用户态如何利用原子操作避免锁？", "用 `std::atomic<T>` 的无锁操作：① 单生产者单消费者队列：`atomic<size_t> head, tail` + `release`/`acquire` 内存序。② 引用计数：`shared_ptr` 底层是 `atomic` 引用计数。③ 心跳/序列号：`atomic<uint64_t>` + `memory_order_relaxed`。关键是选对内存序：`relaxed`（无屏障）→ `acquire`/`release`（一对屏障）→ `seq_cst`（全屏障，最安全最慢）。"),
        ],
    ),
    f"{CH5}/section-4-自旋锁.md": (
        [
            "在持有 spinlock 时调用睡眠函数——会死锁或 panic（`BUG: scheduling while atomic`）",
            "以为 spinlock 会自动关中断——普通 `spin_lock()` 不关中断，中断上下文需用 `spin_lock_irqsave()`",
            "混淆 `spin_lock()` 和 `spin_lock_bh()`——前者只禁抢占，后者还禁 softirq",
        ],
        [
            ("`spin_lock()` / `spin_lock_irq()` / `spin_lock_irqsave()` / `spin_lock_bh()` 的区别？", "`spin_lock()`：禁抢占。`spin_lock_irq()`：禁抢占 + 关本地中断（如果知道调用前中断是开的）。`spin_lock_irqsave(flags)`：禁抢占 + 保存中断状态 + 关中断（最安全，推荐）。`spin_lock_bh()`：禁抢占 + 禁 softirq（不关 hard IRQ）。选择原则：进程上下文 → `spin_lock()` 或 `spin_lock_irqsave()`（如中断也访问）。中断上下文 → `spin_lock_irqsave()`。softirq 上下文 → `spin_lock_bh()`。"),
            ("为什么持有 spinlock 时不能睡眠？", "Spinlock 假设等待者会忙等（spin），不释放 CPU。如果持锁者睡眠（`schedule()`），等待者会无限 spin 浪费 CPU。更严重的是：① `schedule()` 在 `preempt_count > 0` 时 panic。② 如果睡眠后切换到的进程也请求同一锁 → 死锁。RT 内核把 spinlock 变成可睡眠锁后，这个限制不成立，但吞吐下降。"),
            ("HFT 用户态怎么模拟 spinlock 的效果？", "用户态没有真正的 spinlock（不关中断/不禁抢占），但可以用：① `std::atomic_flag` + `test_and_set` 自旋等待（短临界区，<100ns）。② `sched_yield()` + 重试（中等临界区）。③ `spinlock` 如果持有时间 >1us 应改用 `futex`/`mutex`（避免浪费 CPU）。关键是测量持有时间：`perf stat -e instructions` 在持锁前后计数。"),
        ],
    ),
    f"{CH5}/section-5-顺序锁与RCU.md": (
        [
            "把 ULK 讲的 RCU 当现代版——现代有 Tree RCU、Sleepable RCU (SRCU)、Tasks RCU，API 完全不同",
            "以为 seqlock 是通用读写锁——seqlock 只适合「写少读多 + 读端可容忍重试」的场景",
            "在 RCU 读端临界区中睡眠——普通 RCU (`rcu_read_lock()`) 不能睡眠，只有 SRCU (`srcu_read_lock()`) 可以",
        ],
        [
            ("RCU（Read-Copy-Update）的核心思想是什么？", "读端无锁：`rcu_read_lock()` 只禁抢占（无开销），读者直接访问旧数据。写端复制：写者复制一份数据，修改副本，然后用 `rcu_assign_pointer()` 原子替换指针。回收：写者调用 `synchronize_rcu()` 等待所有读端退出，再释放旧数据。关键：读者看到的是旧版本或新版本，绝不会是中间状态。适合：读多写少的数据结构（路由表、VMA 链表）。"),
            ("Tree RCU 和 ULK 讲的 RCU 有什么区别？", "ULK 讲的是经典 RCU（单一 grace period 检测）。Tree RCU（2.6.29+）把 CPU 组织成树形结构，每层汇报 quiescent state，避免全局扫描所有 CPU。在 1000+ CPU 系统上，Tree RCU 的 grace period 从秒级降到毫秒级。API 变化：`synchronize_rcu()` 仍可用，但底层实现完全不同。新增 `call_rcu()`（异步回收）和 `rcu_barrier()`（等所有 pending 回收完成）。"),
            ("seqlock 在什么场景下比 RCU 更合适？", "Seqlock 适合：① 数据结构简单（几个计数器/时间戳）。② 写频率高于 RCU 的舒适区（RCU 写端开销大）。③ 读端可以容忍偶尔重读。典型用法：`jiffies` 和 `getnstimeofday()`——写者更新时间戳时递增 sequence number，读者检查前后 sequence 一致则读成功。不适合复杂数据结构（链表/树），因为读端重试代价高。"),
        ],
    ),
    f"{CH5}/section-6-信号量与完成变量.md": (
        [
            "混淆 `semaphore` 和 `mutex`——mutex 有归属（只有持有者能解锁），semaphore 无归属",
            "以为 `completion` 和 `semaphore` 等价——completion 是一次性信号（complete 后不可复用），semaphore 可重复",
            "在中断上下文中调用 `down()`——`down()` 会睡眠，中断上下文只能用 `down_trylock()`",
        ],
        [
            ("`mutex` 和 `semaphore` 的关键区别？", "mutex：① 有归属（`task_struct *owner`），只有持锁者能 `mutex_unlock()`。② 支持优先级继承（`rt_mutex`，防优先级反转）。③ 支持 `lockdep` 调试。semaphore：① 无归属，任何人可以 `up()`。② 初始值可 >1（计数信号量）。③ 无优先级继承。内核新代码推荐用 `mutex`，`semaphore` 只在需要计数语义时使用。"),
            ("`completion` 为什么比 `semaphore` 更适合「等待一次性事件」？", "① completion 语义清晰：`init_completion()` → `wait_for_completion()` → `complete()`，只用于一次性通知。② 防止误用：semaphore 可被多次 `up()`，completion 的 `complete()` 通常只调一次。③ 支持超时：`wait_for_completion_timeout()`。④ 支持中断可中断：`wait_for_completion_interruptible()`。驱动初始化等待硬件就绪是最典型场景。"),
            ("HFT 中如何避免 `mutex`/`semaphore` 引起的延迟？", "① 避免在热路径上持锁——用无锁数据结构或 per-CPU 变量。② 如果必须持锁，用 `spinlock_t` + 短临界区（<1us），避免 mutex 的调度开销。③ 用 `futex` 替代 `pthread_mutex`（减少内核态往返）。④ `rt_mutex` 的优先级继承防优先级反转——高优先级交易线程不会被低优先级线程阻塞的锁卡住。⑤ `perf lock` 分析锁等待时间。"),
        ],
    ),
    f"{CH5}/section-7-选型与实例.md": (
        [
            "在所有场景都用 spinlock——spinlock 只适合极短临界区，长临界区用 mutex（可睡眠不浪费 CPU）",
            "以为 RCU 总是最优——RCU 读端无开销但写端开销大（等 grace period），写频繁时不适合",
            "忽略 lockdep——`CONFIG_LOCKDEP=y` 在开发阶段能检测死锁/锁顺序反转，生产阶段关掉",
        ],
        [
            ("给定一个场景，如何选择同步原语？", "中断上下文 + 短临界区 → `spinlock_irqsave()`。softirq + 短临界区 → `spinlock_bh()`。进程上下文 + 短临界区（<1us） → `spinlock()`。进程上下文 + 长临界区（>1us） → `mutex()`。读多写少 + 简单数据 → `seqlock` 或 RCU。读多写少 + 复杂数据结构 → RCU。一次性等待 → `completion`。引用计数 → `refcount_t`。"),
            ("`lockdep` 能检测哪些问题？怎么使用？", "检测：① 死锁（A→B 和 B→A 锁顺序反转）。② 同一个锁重复加锁。③ 在中断上下文中持有可睡眠锁。④ 锁的 IRQ 安全性不匹配（进程上下文用 `spinlock()`，中断上下文用 `spinlock_irqsave()` 但两者锁定同一 `spinlock` → 死锁风险）。使用：`CONFIG_LOCKDEP=y` 编译内核，`echo 1 > /proc/sys/kernel/lock_stat` 开启统计，`cat /proc/lock_stat` 查看结果。"),
            ("HFT 用户态有没有类似 lockdep 的工具？", "有：① ThreadSanitizer (`-fsanitize=thread`)：编译时插桩，运行时检测数据竞争。② Helgrind (Valgrind)：无需重编译，但慢 20-50x。③ `perf lock`：内核级锁竞争分析（含 `futex`）。④ `bpftrace`：`tracepoint:syscalls:sys_enter_futex` 追踪 futex 等待。HFT 开发应 CI 中跑 TSan，上线前跑 `perf lock` 确认无异常锁竞争。"),
        ],
    ),
}

# ============================================================
# Ch7 Process Scheduling (6 notes)
# ============================================================
CH7 = "chapter-07-process-scheduling/notes"
CH7_NOTES = {
    f"{CH7}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 O(1) 调度器当现代版——2.6.23 起 CFS 取代 O(1)，6.6 起 EEVDF 取代 CFS",
            "以为 nice 值直接决定 CPU 时间比例——nice 通过 `static_prio` 查 `sched_prio_to_weight[]` 表得到权重，权重决定比例",
            "混淆 SCHED_OTHER 和 SCHED_FIFO/RR——OTHER 是普通分时（CFS/EEVDF），FIFO/RR 是实时策略（RT 调度器）",
        ],
        [
            ("ULK 讲的 O(1) 调度器在现代内核中还存在吗？", "不存在。演进链：O(1) 调度器（2.6.0-2.6.22）→ CFS（2.6.23-6.5）→ EEVDF（6.6+）。O(1) 用优先级数组 + 时间片，CFS 用 vruntime + 红黑树，EEVDF 用虚拟截止时间 + eligibility。ULK 的 `recalc_task_prio()`、优先级数组等概念已完全过时。"),
            ("CFS 和 EEVDF 的核心理念有什么区别？", "CFS（完全公平）：按权重瓜分 CPU，vruntime 记账，选 vruntime 最小的。目标：完美公平。EEVDF（最早合格虚拟截止时间）：每个任务有资格时间（eligibility）和截止时间（deadline），选最早截止且有资格的。目标：公平 + 延迟保证。EEVDF 在延迟敏感场景（如交互/媒体）表现更好，且解决了 CFS 的某些公平性 corner case。"),
            ("HFT 应该用哪个调度策略？", "`SCHED_FIFO`（实时，优先级 1-99）。交易线程设 `SCHED_FIFO` + 绑核 + `mlockall`，确保不被普通进程抢占。注意：① `SCHED_FIFO` 线程不会自动让出 CPU（除非阻塞/主动 yield/更高优先级抢占）。② 需 `CAP_SYS_NICE` 或 root。③ RT 线程 bug 可能锁死 CPU，设 `rlimit_rttime` 限制。④ `isolcpus` 隔离防其他 RT 任务竞争。"),
        ],
    ),
    f"{CH7}/section-2-调度策略与抢占.md": (
        [
            "混淆 SCHED_FIFO 和 SCHED_RR——FIFO 无时间片（跑到阻塞/被更高优先级抢占），RR 有时间片（轮转）",
            "以为 SCHED_DEADLINE 是 CFS 的一部分——DEADLINE 是独立的调度类（EDF 算法），优先级最高",
            "在 RT 策略下以为 nice 值还有效——RT 策略不看 nice，看 rt_priority（1-99）",
        ],
        [
            ("SCHED_OTHER / SCHED_FIFO / SCHED_RR / SCHED_DEADLINE 四个策略的区别？", "OTHER：普通分时，CFS/EEVDF 调度，nice [-20,19] 影响权重。FIFO：实时，同优先级内 FIFO，无时间片，跑到阻塞/被更高优先级抢占。RR：实时，同优先级内轮转，有时间片（默认 100ms）。DEADLINE：基于 EDF（Earliest Deadline First），需要指定 runtime/deadline/period，优先级最高。"),
            ("`rt_runtime_us` 和 `rt_period_us` 是什么？为什么需要？", "RT 线程（FIFO/RR）可以无限占用 CPU，导致系统无响应。`rt_runtime_us`（默认 950000us）和 `rt_period_us`（默认 1000000us）限制：每 `rt_period_us` 时间窗口内，RT 线程最多跑 `rt_runtime_us`。超过后 RT 线程被节流（throttled），让 CFS 运行。可通过 `/proc/sys/kernel/sched_rt_runtime_us` 调整。HFT 设 `-1` 禁用节流（需要确保 RT 线程不会 bug）。"),
            ("HFT 用 SCHED_FIFO 时常见的延迟来源有哪些？", "① 中断（hard IRQ）仍可打断 RT 线程——用 `isolcpus` + 中断重定向。② softirq（ksoftirqd）——用 `nohz_full`。③ RT 线程间的锁竞争——用无锁设计。④ 内存分配（page fault）——`mlockall` + 预分配。⑤ CPU 频率调节——`cpufreq=governor performance` 锁定最高频率。⑥ SMT/超线程——`nosmt` 禁用。"),
        ],
    ),
    f"{CH7}/section-3-调度器数据结构.md": (
        [
            "把 ULK 讲的 `runqueue` 结构当现代版——6.x 用 `cfs_rq`/`rt_rq`/`dl_rq` 分层结构，且 EEVDF 进一步改了 CFS 部分",
            "混淆 `sched_entity` 和 `task_struct`——CFS 操作 `sched_entity`（可代表组/进程），`task_struct` 内嵌 `sched_entity`",
            "以为运行队列是全局的——现代内核 per-CPU 运行队列（`rq`），负载均衡在 CPU 间迁移任务",
        ],
        [
            ("现代调度器的运行队列层级结构是什么？", "每 CPU 一个 `struct rq`，内含三个子队列：`cfs_rq`（CFS/EEVDF 任务）、`rt_rq`（RT 任务）、`dl_rq`（DEADLINE 任务）。调度时按优先级选：DEADLINE > RT > CFS。`rq` 还包含 `curr`（当前运行任务）、`clock`（CPU 时钟）、`nr_running`（总就绪数）。ULK 讲的全局 `runqueue` 数组已被 per-CPU `rq` 取代。"),
            ("`sched_entity` 的作用是什么？为什么 CFS 不直接操作 `task_struct`？", "`sched_entity` 是调度实体，嵌在 `task_struct` 中。它支持组调度（cgroup）——一组进程作为一个 `sched_entity` 参与 CFS 调度，组内再按 CFS 分配。`sched_entity` 包含 `vruntime`、`load`（权重）、`rb_node`（红黑树节点）、`cfs_rq`（所属队列）。CFS 通过操作 `sched_entity` 实现进程/组/层级调度。"),
            ("HFT 如何减少调度器对延迟的影响？", "① `isolcpus=N` 隔离 N 号核——该核上不跑普通任务，调度器几乎不触发。② `SCHED_FIFO` + 绑核——RT 线程独占该核，不会被 CFS 任务抢占。③ `nohz_full=N`——停止定时器中断，消除 `scheduler_tick()`。④ `rcu_nocbs=N`——RCU 回调迁移到其他核。⑤ `tuned profile=network-latency`——一键调优。"),
        ],
    ),
    f"{CH7}/section-4-调度算法与核心函数.md": (
        [
            "把 ULK 的 `recalc_task_prio()` / `schedule()` 当现代版——CFS 的 `schedule()` 逻辑完全不同（选红黑树最左节点）",
            "以为只有时间片耗尽才调度——CFS 还有唤醒抢占（唤醒的 vruntime 更小时直接抢占）",
            "混淆 `scheduler_tick()` 和 `schedule()`——前者更新统计 + 设标记，后者执行实际切换",
        ],
        [
            ("CFS 的 `schedule()` 核心流程是什么？", "① `pick_next_task()`：从 `cfs_rq` 红黑树取最左节点（vruntime 最小）→ `sched_entity` → `task_struct`。② `put_prev_task()`：当前任务更新 vruntime，放回红黑树（如仍就绪）。③ `context_switch()`：切换 `mm_struct`（`switch_mm()`）+ 寄存器（`switch_to()`）。CFS 的 `schedule()` 比 O(1) 简单——不用维护优先级数组，红黑树 O(log n) 选下一个。"),
            ("唤醒抢占的判定条件是什么？", "唤醒的任务 Q 的 vruntime 比 current 的 vruntime 小一定阈值时，Q 抢占 current。阈值 = `sched_wakeup_granularity`（默认 1ms，可调）。效果：交互任务（键盘输入）唤醒后 vruntime 小（睡眠时不增长），立即抢占后台任务。这是 CFS 交互响应好的核心机制。EEVDF 改用 eligibility + deadline 判定，更精确。"),
            ("HFT 如何测量调度延迟？", "① `cyclictest -p 99 -t 1 -a [core]`：RT 线程测最大调度延迟。② `perf sched`：记录调度事件，分析延迟分布。③ `bpftrace -e 'tracepoint:sched:sched_switch { ... }'`：追踪上下文切换。④ `/proc/[pid]/sched`：查看调度统计。目标：HFT 热路径调度延迟 <1us（绑核 + SCHED_FIFO + isolcpus）。"),
        ],
    ),
    f"{CH7}/section-5-SMP运行队列平衡.md": (
        [
            "以为负载均衡是即时的——负载均衡周期性执行（tick 触发），有迁移延迟",
            "混淆 load balance 和 task migration——load balance 在 CPU 间迁移任务，HFT 要避免迁移（cache miss）",
            "以为 `sched_setaffinity()` 就能保证不被迁移——RT 线程可以，CFS 线程可能在负载均衡时被迁移到同 affinity 集合内的其他核",
        ],
        [
            ("SMP 负载均衡的触发时机和策略？", "触发：① `scheduler_tick()`（周期性，每 ~1ms 检查）。② CPU idle 时主动拉任务（idle balance）。③ `kick_offline_cpu()` 等特殊路径。策略：① domain 层级（SMT → MC → DIE → NUMA），从低到高检查不平衡。② 计算各 CPU 的 `load`（基于 `sched_entity` 权重和运行时间）。③ 如果不平衡度超阈值，从最忙的 CPU 迁移任务到最闲的 CPU。"),
            ("HFT 为什么要避免任务在 CPU 间迁移？", "迁移导致：① L1/L2/L3 cache 全部 miss（冷启动延迟）。② TLB 刷新（`switch_mm()` 写 `CR3`）。③ NUMA 跨节点访问延迟（local → remote 多 ~100ns）。一次迁移的延迟惩罚可达 10-100us。避免方法：`sched_setaffinity` 绑定单核 + `isolcpus` 隔离 + `numactl --membind` 绑定本地内存。"),
            ("`isolcpus` 和 `cpuset` cgroup 有什么区别？", "`isolcpus=N`：启动参数，从调度器可运行 CPU 集合中移除 N 号核，普通任务不会调度到 N。需要手动 `taskset` 或 `sched_setaffinity` 把 RT 线程放到 N。`cpuset` cgroup：运行时配置，创建 cgroup 设置 `cpuset.cpus=N`，把任务加入该 cgroup。`isolcpus` 更彻底（连 kworker/RCU 都不走），`cpuset` 更灵活（可运行时调整）。"),
        ],
    ),
    f"{CH7}/section-6-调度相关系统调用.md": (
        [
            "混淆 `nice()` 和 `setpriority()`——`nice()` 只能相对当前值调整，`setpriority()` 可直接设置绝对值",
            "以为 `sched_setscheduler()` 需要 root——现代内核需要 `CAP_SYS_NICE` capability，非 root 也可",
            "在 RT 策略下调用 `sched_yield()`——FIFO 策略下 yield 会把线程移到同优先级队列末尾，可能不立即重新运行",
        ],
        [
            ("`nice(inc)` / `setpriority(PRIO_PROCESS, pid, prio)` / `sched_setscheduler()` 的区别？", "`nice(inc)`：当前进程 nice += inc（受 RLIMIT_NICE 限制），返回新 nice 值。`setpriority(PRIO_PROCESS, pid, prio)`：设置指定进程的 nice 值（绝对值）。`sched_setscheduler(pid, SCHED_FIFO, &param)`：切换调度策略和 RT 优先级。HFT 用 `sched_setscheduler(SCHED_FIFO, .sched_priority=99)` 设置最高 RT 优先级。"),
            ("`SCHED_FIFO` 下 `sched_yield()` 的行为是什么？", "FIFO 策略：yield 把当前线程移到同优先级队列末尾。如果队列中没有其他同优先级线程，yield 立即返回（线程继续运行）。如果有同优先级线程，下一个线程运行。yield 不会让出给低优先级线程。HFT 不应在 RT 热路径上用 yield——如果需要让出 CPU 说明设计有问题。"),
            ("HFT 如何用 `sched_setaffinity()` + `mlockall()` 建立确定性环境？", "```c\n// 1. 绑核\ncpu_set_t cpuset;\nCPU_ZERO(&cpuset); CPU_SET(2, &cpuset);  // 绑到 2 号核\nsched_setaffinity(0, sizeof(cpuset), &cpuset);\n// 2. RT 优先级\nstruct sched_param sp = { .sched_priority = 99 };\nsched_setscheduler(0, SCHED_FIFO, &sp);\n// 3. 锁内存（防 swap/page fault）\nmlockall(MCL_CURRENT | MCL_FUTURE);\n// 4. 预分配 + 大页\nvoid *p = mmap(NULL, size, PROT_READ|PROT_WRITE,\n               MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_POPULATE,\n               -1, 0);\n```"),
        ],
    ),
}

# ============================================================
# Ch8 Memory Management (4 notes)
# ============================================================
CH8 = "chapter-08-memory-management/notes"
CH8_NOTES = {
    f"{CH8}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 buddy system / slab 当现代版——SLUB 取代 SLAB、`struct folio` 取代 `struct page` 作为基本管理单元",
            "以为 `kmalloc()` 分配的是物理连续内存——是的，但虚拟地址在直接映射区，所以物理连续 = 虚拟连续",
            "混淆 `GFP_KERNEL` 和 `GFP_ATOMIC`——前者可睡眠（可能触发回收），后者不睡眠（紧急分配）",
        ],
        [
            ("ULK Ch8 讲的内存管理在现代内核中有哪些重大变化？", "① SLAB → SLUB（2.6.23 默认）：去掉 per-CPU 的 slab 队列，简化结构，减少内存开销。② `struct page` → `struct folio`（6.1+）：folio 是一组连续的 page，减少遍历开销。③ buddy system 仍存在但 API 更新（`alloc_pages()` → `folio_alloc()`）。④ NUMA 感知增强（per-node 页池）。⑤ MGLRU 取代传统 LRU 回收。"),
            ("`GFP_KERNEL` / `GFP_ATOMIC` / `GFP_DMA` 的区别和使用场景？", "`GFP_KERNEL`：进程上下文分配，可睡眠（允许磁盘 I/O 回收内存），最常用。`GFP_ATOMIC`：不睡眠（中断/softirq/持锁时），从紧急预留池分配，可能失败。`GFP_DMA`：要求 <16MB 物理地址（老 DMA 设备）。组合：`GFP_KERNEL | __GFP_NOWARN`。HFT 内核模块在热路径应用 `GFP_ATOMIC`（但不能失败），更好的做法是预分配 `mempool`。"),
            ("HFT 用户态如何选择内存分配策略？", "① 小对象：`malloc`/`new`（glibc ptmalloc2 或 jemalloc）。② 大块连续：`mmap(MAP_ANONYMOUS | MAP_HUGETLB)`（2MB 大页）。③ 物理连续（DMA 场景）：`/dev/hugepages` + `mmap`。④ 内存池：预分配 + 自管理，避免运行时分配。⑤ 零拷贝：`mmap` 映射文件/设备。关键：`mlockall(MCL_CURRENT | MCL_FUTURE)` 防止换出。"),
        ],
    ),
    f"{CH8}/section-2-页框管理.md": (
        [
            "把 ULK 讲的 buddy system 当不变的事实——6.x 的 buddy 仍存在但 API 大改（`folio` 系列），且支持 MGLRU 回收",
            "混淆 `alloc_pages()` 和 `__get_free_pages()`——前者返回 `struct page*`，后者返回虚拟地址",
            "以为 `free_pages()` 和 `put_page()` 等价——`free_pages()` 是 buddy API，`put_page()` 是引用计数减一",
        ],
        [
            ("buddy system 的核心思想和优缺点？", "核心：将空闲页按 2^n 大小分块（order 0 = 4KB, order 1 = 8KB, ..., order 10 = 4MB）。分配时找最小满足的块，大了就分裂。释放时检查 buddy（相邻同大小块）是否空闲，空闲则合并。优点：① 快速（O(log N)）。② 避免外碎片（大块需求能满足）。缺点：① 内碎片（请求 5KB 分配 8KB）。② 不适合小对象（用 slab/slob 补充）。"),
            ("现代内核 `struct folio` 相比 `struct page` 解决了什么问题？", "`struct page` 每个 4KB 页一个，64GB 内存有 16M 个 `struct page`（每个 64 字节 = 1GB 开销）。Folio 将多个连续页作为一个管理单元，减少 `struct page` 数量和遍历开销。API：`folio_alloc()` / `folio_free()` / `folio_address()`。对文件系统/页缓存收益最大（一个 folio 管理多页，减少锁竞争和树操作）。ULK 时代没有 folio 概念。"),
            ("HFT 如何利用大页（huge page）减少 TLB miss？", "① `mmap(MAP_HUGETLB, ...)`：匿名大页（2MB/1GB），TLB 一个条目覆盖 512/262144 个 4KB 页。② `madvise(MADV_HUGEPAGE)`：让内核对普通匿名映射尝试合并为透明大页（THP）。③ `/sys/kernel/mm/transparent_hugepage/enabled=always`。注意：THP 可能在后台触发 `khugepaged` 整理内存，导致延迟尖峰。HFT 优先用显式 `MAP_HUGETLB`。"),
        ],
    ),
    f"{CH8}/section-3-Slab分配器.md": (
        [
            "把 ULK 讲的 SLAB 当现代默认——SLUB 已取代 SLAB 成为默认，SLOB 用于嵌入式小内存",
            "混淆 SLAB/SLUB/SLOB——SLAB（复杂、per-CPU 队列）、SLUB（简化、性能好）、SLOB（极小、嵌入式）",
            "以为 `kmalloc()` 是唯一的内核分配器——还有 `vmalloc()`（虚拟连续）、`alloc_pages()`（页级）、`kmem_cache_alloc()`（专用 slab）",
        ],
        [
            ("SLAB、SLUB、SLOB 三者的区别？为什么 SLUB 成为默认？", "SLAB：ULk 时代默认，复杂的多层 per-CPU 队列结构，管理开销大。SLUB（2.6.23+ 默认）：简化结构，每个 slab 只一个 per-CPU page，减少元数据开销，调试友好。SLOB：极简，用于内存极小的嵌入式系统（<16MB）。SLUB 胜出原因：① 更低元数据开销。② 更好的 NUMA 性能。③ 内联 freelist 简化。④ `slabinfo` 工具兼容。"),
            ("`kmalloc()` / `vmalloc()` / `kmem_cache_alloc()` 的区别和选择？", "`kmalloc(size, flags)`：物理连续 + 虚拟连续（直接映射区），限制在 ~32MB（MAX_ORDER），快。`vmalloc(size)`：虚拟连续但物理不连续，可分配大块（GB 级），慢（需建页表 + TLB 压力）。`kmem_cache_alloc(cache, flags)`：从专用 slab cache 分配固定大小对象，最快，零内碎片。选择：小对象 → `kmem_cache_alloc`；通用小/中 → `kmalloc`；大块非连续 → `vmalloc`。"),
            ("HFT 用户态如何实现类似 slab 的对象池？", "```c\n// 预分配固定大小对象池\nstruct Pool {\n    void *base;       // mmap 预分配\n    size_t obj_size;\n    size_t capacity;\n    std::atomic<size_t> free_idx;\n};\nvoid* alloc(Pool* p) {\n    size_t idx = p->free_idx.fetch_add(1, std::memory_order_relaxed);\n    if (idx >= p->capacity) return nullptr;\n    return (char*)p->base + idx * p->obj_size;\n}\n// 关键：无锁、预分配、cache-line 对齐\n``` 优势：零分配延迟、无系统调用、无锁竞争。"),
        ],
    ),
    f"{CH8}/section-4-非连续内存与vmalloc.md": (
        [
            "以为 `vmalloc()` 和用户态 `malloc()` 类似——`vmalloc()` 建立页表映射非连续物理页，开销远大于 `kmalloc()`",
            "在性能敏感路径用 `vmalloc()`——`vmalloc()` 需要建页表 + 可能触发 TLB shootdown，不适合热路径",
            "混淆 `vmap()` 和 `vmalloc()`——`vmap()` 映射已有 pages，`vmalloc()` 分配新 pages 再映射",
        ],
        [
            ("`vmalloc()` 的工作原理和开销？", "① 在 vmalloc 区预留虚拟地址空间。② 调 `alloc_page()` 分配物理页（可能不连续）。③ 调 `map_vm_area()` 建页表（每 4KB 一个 PTE）。④ TLB 需要 flush（新映射）。开销：比 `kmalloc()` 慢 10-100 倍（页表建立 + TLB flush）。优势：可分配大块（不受 MAX_ORDER 限制）、不要求物理连续。适合：内核模块加载、大缓冲区分配。不适合：热路径。"),
            ("为什么 `vmalloc()` 不适合 HFT 内核模块热路径？", "① 分配时建页表 = 多次原子写 + TLB flush，微秒级开销。② 访问时 TLB miss（页表 walk），纳秒级开销。③ `vfree()` 需要等 RCU grace period + TLB shootdown IPI，可能毫秒级。如果内核模块必须分配大块，应在初始化时 `vmalloc` 一次，之后用内存池管理。"),
            ("现代内核如何优化 `vmalloc()` 的性能？", "① `vmalloc_to_page()` 用 `virt_to_page()` 快速路径（如果地址在直接映射区）。② lazy TLB flush：延迟到下一个 `schedule()` 时统一 flush。③ `vfree_atomic()`：异步释放（不等 grace period）。④ `__vmalloc_node_range()`：NUMA 感知分配。但这些优化仍无法消除基本开销——热路径应避免 `vmalloc`。"),
        ],
    ),
}

# ============================================================
# Ch9 Process Address Space (6 notes)
# ============================================================
CH9 = "chapter-09-process-address-space/notes"
CH9_NOTES = {
    f"{CH9}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 VMA 红黑树当现代版——6.1 起 maple tree 取代红黑树管理 VMA",
            "以为进程地址空间只有代码+数据+堆+栈——还有 mmap 区、vdso、[vvar]、[heap] 等，布局复杂",
            "混淆 `mm_struct` 和 `task_struct`——`task_struct` 是进程描述符，`mm_struct` 是地址空间描述符",
        ],
        [
            ("ULK Ch9 的 VMA 管理在现代内核中有什么变化？", "最大变化：VMA 查找结构从红黑树 + 链表改为 **maple tree**（6.1+）。原因：① 红黑树在大量 VMA（如数据库进程数百个 mmap）时查找慢（O(log n)）。② maple tree 是 B-tree 变体，缓存友好，查找 O(log n) 但常数更小。③ maple tree 原生支持范围查询（`find_vma()` 场景）。API 不变：`find_vma()` / `find_vma_intersection()` 内部改用 maple tree。"),
            ("一个进程的 `mm_struct` 中有哪些关键字段？", "`pgd`（PGD 物理地址）、`mmap`（VMA 链表头）、`mm_rb`/`mm_mt`（VMA 树/maple tree）、`mmap_base`（mmap 区起始地址）、`total_vm`（总页数）、`locked_vm`（mlock 页数）、`def_flags`、`mmap_sem`/`mmap_lock`（读写锁）、`cpu_vm_mask`（CPU TLB 掩码）。ULK 时代用 `mmap_sem`，6.x 改名 `mmap_lock`。"),
            ("HFT 如何优化进程地址空间布局？", "① `mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定所有页，防 swap。② 预 `mmap` 所有内存区域（避免运行时 VMA 分配）。③ 使用大页减少 TLB 条目。④ `prctl(PR_SET_THP_DISABLE)` 禁用 THP（避免 `khugepaged` 整理引起的延迟）。⑤ 检查 `/proc/[pid]/maps` 确认无意外映射。⑥ NUMA 绑定：`numactl --membind=0`。"),
        ],
    ),
    f"{CH9}/section-2-内存描述符.md": (
        [
            "混淆 `mm_struct` 的引用计数——`mm_count` 是 `mm_struct` 本身的引用，`mm_users` 是使用该 mm 的线程数",
            "以为内核线程没有 `mm_struct`——内核线程 `mm = NULL`，但 `active_mm` 借用前一个用户进程的",
            "在持有 `mmap_lock` 时做耗时操作——`mmap_lock` 是读写锁，写锁会阻塞所有 `page fault`",
        ],
        [
            ("`mm_users` 和 `mm_count` 的区别？", "`mm_users`：使用该地址空间的线程数（`clone(CLONE_VM)` 共享 mm 时 +1）。降到 0 时释放 `mm_struct` 的用户资源（页表、VMA）。`mm_count`：`mm_struct` 本身的引用（内核模块/`active_mm` 持有）。降到 0 时释放 `mm_struct` 结构体本身。关系：`mm_users` 每次降为 0 时 `mm_count` -1。所以 `mm_users > 0` 时 `mm_count >= 1`。"),
            ("`mmap_lock`（原 `mmap_sem`）在 6.x 内核中有什么变化？", "① 改名 `mmap_sem` → `mmap_lock`（更准确表达它是 lock 不是 semaphore）。② 从 RWSEM 改为可配置的 `rw_semaphore`。③ 新增 `mmap_read_trylock()` / `mmap_write_trylock()` 非阻塞接口。④ `page fault` 路径用 `mmap_read_lock()`（读锁，不阻塞其他 fault）。⑤ `mmap`/`munmap`/`mprotect` 用 `mmap_write_lock()`（写锁，阻塞所有 fault）。HFT 热路径避免 `mprotect`（会持写锁阻塞 fault）。"),
            ("如何查看进程的 `mm_struct` 状态？", "① `/proc/[pid]/status` 中的 `VmPeak`/`VmSize`/`VmRSS`/`VmData`/`VmStk`/`VmExe`。② `/proc/[pid]/maps`：所有 VMA。③ `/proc/[pid]/smaps`：每个 VMA 的详细统计（RSS、PSS、anon、swap）。④ `/proc/[pid]/statm`：页级别统计。⑤ `pmap [pid]`：格式化输出。HFT 诊断内存问题用 `smaps` 看每个映射的 RSS 和大页状态。"),
        ],
    ),
    f"{CH9}/section-3-内存区VMA.md": (
        [
            "把 ULK 的 VMA 红黑树当现代版——6.1+ 用 maple tree，VMA 结构体也有变化",
            "混淆 VMA 的 `vm_start`/`vm_end` 和实际物理页——VMA 描述虚拟地址范围，物理页在访问时才分配（demand paging）",
            "以为所有 mmap 都分配物理内存——匿名 mmap 只分配 VMA，物理页在首次访问时通过 page fault 分配",
        ],
        [
            ("`vm_area_struct`（VMA）的核心字段有哪些？", "`vm_start`/`vm_end`（虚拟地址范围）、`vm_flags`（权限：`VM_READ`/`VM_WRITE`/`VM_EXEC`/`VM_MAY*`）、`vm_page_prot`（页表权限）、`vm_ops`（VMA 操作函数表：`fault`/`open`/`close`）、`vm_file`（文件映射时指向 file）、`vm_pgoff`（文件内偏移）、`anon_vma`（匿名映射的反向映射）。ULK 时代还有 `vm_rb`（红黑树节点），6.1+ 改为 maple tree 节点。"),
            ("匿名 mmap 和文件 mmap 的 VMA 有什么区别？", "匿名 mmap：`vm_file = NULL`，`vm_ops = NULL`（或匿名 `vm_ops`），物理页在首次写时分配（`do_anonymous_page()`）。文件 mmap：`vm_file != NULL`，`vm_ops = file->f_op->vm_ops`（如 `ext4_file_vm_ops`），物理页从 page cache 读取（`vm_ops->fault()` → `filemap_fault()`）。匿名 mmap 用于堆/栈/`malloc` 大块；文件 mmap 用于共享库/内存映射 I/O。"),
            ("HFT 如何用 `mmap` 建立零拷贝数据通道？", "① 共享内存：`mmap(MAP_SHARED | MAP_ANONYMOUS, ...)` 在父子进程间共享。② 文件映射：`mmap(MAP_SHARED, fd, ...)` 映射文件，多进程共享同一物理页。③ `memfd_create()` + `mmap`：无文件支持的共享内存（`/dev/shm`）。④ huge page 共享：`mmap(MAP_SHARED | MAP_HUGETLB, ...)`。关键：共享内存 + 内存屏障（`std::atomic`）实现无锁 IPC，延迟 <100ns（vs pipe/socket 的 us 级）。"),
        ],
    ),
    f"{CH9}/section-4-缺页异常.md": (
        [
            "把所有 page fault 当错误——demand paging 和 COW 是正常机制，不是错误",
            "混淆 major fault 和 minor fault——major 要读磁盘（慢），minor 在内存中解决（快但仍纳秒级）",
            "在 HFT 热路径触发 page fault——page fault 开销 1-10us，对 HFT 是灾难",
        ],
        [
            ("page fault 的完整处理流程？", "① CPU 触发 #PF，`CR2` = 故障地址。② `do_page_fault()` → `handle_mm_fault()`。③ 查找 VMA（maple tree）：无 VMA → `SIGSEGV`。④ VMA 存在但权限不符（如写只读 VMA）→ `SIGSEGV`。⑤ PTE 不存在 + 匿名页 → `do_anonymous_page()`（分配零页）。⑥ PTE 不存在 + 文件页 → `do_fault()` → `vm_ops->fault()`。⑦ PTE 存在但只读 + 写操作 → `do_wp_page()`（COW）。⑧ 返回 0 = 成功，返回非 0 = `SIGSEGV`/OOM。"),
            ("HFT 如何消除热路径上的 page fault？", "① `mlockall(MCL_CURRENT | MCL_FUTURE)`：锁定所有当前页 + 未来映射的页，禁止 swap。② `MAP_POPULATE`：`mmap` 时预建页表，物理页立即分配。③ `memset(buf, 0, size)`：强制触发所有页的 COW/minor fault，之后不再 fault。④ 大页（`MAP_HUGETLB`）：减少 TLB miss + 减少 PTE 数量。⑤ 检查：`perf stat -e page-faults ./hft_engine` 应显示 0 fault。"),
            ("major fault 和 minor fault 分别在什么情况下发生？对 HFT 的影响？", "Minor fault：① demand paging（PTE 不存在但物理页在内存，如首次访问匿名页）。② COW（fork 后子进程首次写）。处理时间：~1-5us。Major fault：① 从 swap 读入。② 文件映射页不在 page cache（需磁盘 I/O）。处理时间：~毫秒。HFT 要求两者都为 0：`mlockall` 防 swap，`MAP_POPULATE` + 预 `read()` 填充 page cache。"),
        ],
    ),
    f"{CH9}/section-5-请求调页.md": (
        [
            "以为 `mmap` 后内存就分配好了——demand paging 下，物理页在首次访问时才通过 page fault 分配",
            "混淆 `MAP_POPULATE` 和 `mlockall`——前者预建页表（仍可被 swap），后者锁定物理页（不可 swap）",
            "在 swap 频繁的系统中运行 HFT——swap-in 是 major fault（毫秒级），必须禁用 swap",
        ],
        [
            ("demand paging 的优势和代价？", "优势：① 节省物理内存（未访问的页不分配）。② 加快 `fork`/`exec`（不立即分配所有页）。③ 支持超量分配（overcommit）。代价：① 首次访问触发 page fault（~1-5us 延迟）。② 可能触发 swap（major fault，毫秒级）。③ 不可预测的延迟（HFT 大忌）。HFT 应在初始化时消除 demand paging：`MAP_POPULATE` + `mlockall` + 预 `memset`。"),
            ("`overcommit_memory` 的三种模式？HFT 应该用哪个？", "0（启发式）：内核估算可用内存，可能拒绝大额分配。1（总是允许）：任何 `malloc` 都成功（直到 OOM kill）。2（严格）：`total_swap + total_ram * ratio` 限制。HFT 应设 `vm.overcommit_memory=2` + `vm.overcommit_ratio=90`：防止其他进程超量分配挤压 HFT 内存。同时 `swapoff -a` 禁用 swap。"),
            ("如何预填充所有页表消除 demand paging？", "```c\n// 方法 1: MAP_POPULATE\nvoid *p = mmap(NULL, size, PROT_READ|PROT_WRITE,\n              MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE, -1, 0);\n// 方法 2: 显式 touch 每页\nchar *p = mmap(...);\nfor (size_t i = 0; i < size; i += 4096)\n    p[i] = 0;  // 触发 minor fault\n// 方法 3: 大页 + MAP_POPULATE\nmmap(NULL, size, ..., MAP_HUGETLB | MAP_POPULATE, ...);\nmlockall(MCL_CURRENT | MCL_FUTURE);  // 锁定防 swap\n```"),
        ],
    ),
    f"{CH9}/section-6-写时复制与堆.md": (
        [
            "以为 COW 是零开销——COW 首次写触发 page fault + 分配新物理页 + 复制内容，开销 ~1-5us/页",
            "混淆 `brk()` 和 `mmap()`——`brk()` 扩展堆（连续增长），`mmap()` 分配独立 VMA（可任意位置）",
            "在 HFT 中频繁 `malloc`/`free`——glibc malloc 可能调用 `brk`/`mmap` 系统调用，引入延迟",
        ],
        [
            ("COW 的完整流程？fork 后子进程写页时发生什么？", "① `fork()` → `copy_page_range()`：复制 PTE，所有 PTE 设为只读，物理页引用计数 +1。② 子进程写某页 → CPU 触发 #PF（写只读页）。③ `do_wp_page()`：分配新物理页，复制旧页内容，子进程 PTE 指向新页（可写），旧页引用计数 -1。④ 如果旧页引用计数降为 0，旧页被回收。COW 延迟 = 1 page fault + 1 alloc + 1 memcpy ≈ 2-5us/页。"),
            ("`brk()` 和 `mmap()` 在堆管理上的区别？", "`brk(addr)`：设置 program break（堆顶），堆是连续的 VMA，`malloc` 小对象用 `brk`（快、局部性好）。`mmap(NULL, size, ...)`：在 mmap 区创建独立 VMA，`malloc` 大块（>128KB）用 `mmap`（避免堆碎片）。`brk` 只能收缩/扩展堆，`mmap` 可任意分配/释放。glibc `malloc` 默认：小对象→`brk`，大对象→`mmap(MAP_ANONYMOUS)`。HFT 应预分配内存池，避免运行时 `malloc`。"),
            ("HFT 如何避免 `malloc`/`free` 引起的延迟？", "① 预分配内存池：启动时 `malloc` 所有需要的内存，运行时从池中分配（无系统调用）。② `mallopt(M_MMAP_THRESHOLD, ...)` 调大 `brk` 阈值，减少 `mmap` 调用。③ `LD_PRELOAD=libjemalloc.so` 用 jemalloc 替代 glibc malloc（更低的碎片和锁竞争）。④ 完全自管理：`mmap` 一大块 + 自实现 free list。⑤ `malloc_trim(0)` 归还碎片（但会引起 `brk` 系统调用）。"),
        ],
    ),
}

# ============================================================
# Ch10 System Calls (6 notes)
# ============================================================
CH10 = "chapter-10-system-calls/notes"
CH10_NOTES = {
    f"{CH10}/section-1-本章定位.md": (
        [
            "把 ULK 讲的 `int $0x80` 当现代 syscall 入口——x86-64 用 `syscall` 指令，从 MSR_LSTAR 加载入口",
            "以为 syscall 号全局唯一——syscall 号是 per-architecture 的，x86-64 和 ARM64 不同",
            "混淆 syscall 和 libc 函数——`printf` 不是 syscall，`write` 才是；`malloc` 不是 syscall，`brk`/`mmap` 才是",
        ],
        [
            ("ULK Ch10 讲的 syscall 入口在现代 x86-64 上有什么变化？", "① `int $0x80` 软中断被 `syscall` 指令取代（快 3-5 倍，不走 IDT 查表）。② 入口地址从 `MSR_LSTAR` 加载（`entry_SYSCALL_64`）。③ CS/SS 从 `MSR_STAR` 加载，不走 GDT 查表。④ 参数传递从栈改为寄存器（`rdi, rsi, rdx, r10, r8, r9`）。⑤ `sysret` 指令快速返回。⑥ vDSO/vvar 页让部分 syscall（`gettimeofday`/`clock_gettime`）完全在用户态完成。"),
            ("vDSO（Virtual Dynamic Shared Object）是什么？为什么对 HFT 重要？", "vDSO 是内核映射到每个进程的共享库（`[vdso]` VMA），包含 `gettimeofday`/`clock_gettime`/`getcpu` 等函数。这些函数直接读内核映射的 `vvar` 页（内核定时更新），**不触发 syscall**。开销：~20ns（vs syscall 的 ~100-200ns）。HFT 必须用 vDSO 版的 `clock_gettime(CLOCK_MONOTONIC)`，避免 syscall 开销。`ldd` / `getauxval(AT_SYSINFO_EHDR)` 确认 vDSO 可用。"),
            ("如何减少 HFT 中的系统调用数量？", "① 批量化：`io_uring` 替代多次 `read`/`write`（一次 submit 批量 I/O）。② vDSO：`clock_gettime` 走 vDSO 不进内核。③ 预分配：`mlockall` + 内存池避免 `brk`/`mmap`。④ 轮询 vs 中断：DPDK 用户态轮询替代 `epoll_wait`。⑤ `seccomp` 过滤非法 syscall。测量：`strace -c -p [pid]` 统计 syscall 频率。"),
        ],
    ),
    f"{CH10}/section-2-POSIX API与系统调用.md": (
        [
            "混淆 POSIX API 和 syscall——`malloc` 是 POSIX API（glibc 实现），底层调 `brk`/`mmap` syscall",
            "以为每个 libc 函数对应一个 syscall——`printf` → `write`，`fork` → `clone`，映射不是一一对应",
            "以为 syscall 一定经过 libc——可以直接内联汇编 `syscall` 指令绕过 libc（如 Go runtime）",
        ],
        [
            ("POSIX API、libc 函数、syscall 三者的关系？", "POSIX API 是标准接口规范（如 `open`/`read`/`write`/`fork`）。libc（glibc/musl）实现 POSIX API，内部调用 syscall。不是一一对应：`fork()` → `clone()` syscall，`exit()` → `exit_group()` syscall，`printf()` → `write()` syscall（多层封装）。可直接内联汇编调 `syscall` 指令绕过 libc（Go runtime、Crystal 语言这么做，减少 libc 依赖和开销）。"),
            ("为什么 Go runtime 不用 libc 调 syscall？", "① 避免 libc 的信号处理和 TLS 冲突（Go 有自己的 goroutine 调度）。② 减少 libc 封装开销（虽然很小）。③ 独立控制 syscall 行为（如 Go 的非阻塞 I/O 直接用 `epoll` + `nonblock`）。Go 用 `runtime/sys_linux_amd64.s` 中的汇编直接 `syscall` 指令。缺点：无法利用 vDSO（Go 自己实现了 vDSO 解析）。"),
            ("HFT 中如何测量单次 syscall 的开销？", "```c\n// 用 RDTSC 测量\nuint64_t t1 = rdtsc();\nsyscall(SYS_getpid);  // 最简单的 syscall\nuint64_t t2 = rdtsc();\nprintf(\"getpid: %lu cycles (~%lu ns)\\n\", t2-t1, (t2-t1)/3000);  // 3GHz CPU\n```\n典型值：`getpid` ~150ns，`read` ~200ns，`epoll_wait` ~300ns（无事件），`mmap` ~1-5us。"),
        ],
    ),
    f"{CH10}/section-3-分派表与服务例程.md": (
        [
            "把 ULK 的 `sys_call_table` 当唯一分派方式——x86-64 仍用 `sys_call_table` 但入口不同，且加了 spectre 缓解",
            "以为 syscall 号和 ULK 时代一样——6.x 新增了大量 syscall（io_uring、pidfd、clone3 等），编号变了",
            "混淆 `sys_xxx` 和 `SYSCALL_DEFINE1/2/...`——现代内核用 `SYSCALL_DEFINEn` 宏定义 syscall，不是直接 `sys_xxx`",
        ],
        [
            ("现代内核如何定义和注册一个新 syscall？", "```c\nSYSCALL_DEFINE3(my_syscall, int, arg1, char __user *, arg2,\n                size_t, arg3)\n{\n    // 参数验证\n    if (arg1 < 0) return -EINVAL;\n    char *kbuf = kmalloc(arg3, GFP_KERNEL);\n    if (!kbuf) return -ENOMEM;\n    if (copy_from_user(kbuf, arg2, arg3)) {\n        kfree(kbuf); return -EFAULT;\n    }\n    // ... 处理 ...\n    kfree(kbuf);\n    return 0;\n}\n```\n`SYSCALL_DEFINEn` 宏展开后生成 `sys_my_syscall`，自动加入 `sys_call_table`。n = 参数个数。"),
            ("`SYSCALL_DEFINEn` 宏相比直接 `asmlinkage long sys_xxx()` 有什么优势？", "① 类型安全：宏对每个参数做类型检查。② 防溢出攻击：宏将函数名和参数签名组合成唯一符号，增加攻击者预测难度。③ `__SYSCALL_DEFINEx` 内部加 `asmlinkage` + `__visible` + spectre 缓解（`__x86_indirect_thunk`）。④ 自动生成 syscall 表条目。ULK 时代的直接 `asmlinkage long sys_xxx()` 已废弃。"),
            ("如何查找某个 syscall 号？", "① `ausyscall --dump`（audit 包）列出所有 syscall 号。② `/usr/include/asm/unistd_64.h`（x86-64 syscall 号）。③ `man 2 syscall`。④ `/proc/sys/kernel/last_pid` 不是 syscall 号。⑤ `strace -e trace=read cat /dev/null` 查看实际调用。注意：x86-64、ARM64、x86-32 的 syscall 号不同，跨架构不能硬编码。"),
        ],
    ),
    f"{CH10}/section-4-进入与退出.md": (
        [
            "把 ULK 的 `int $0x80` 入口栈帧当现代版——x86-64 `syscall` 指令只存 RIP/FLAGS，不压段寄存器",
            "以为 syscall 自动关中断——`syscall` 指令不修改 IF 标志，中断保持开启（`sysret` 也不改 IF）",
            "混淆用户态栈和内核栈——syscall 切换到内核栈（TSS 中的 `sp0`），用户栈指针存在内核栈上",
        ],
        [
            ("x86-64 `syscall` 指令的精确行为是什么？", "① 保存 `RIP` → `RCX`（返回地址）。② 保存 `RFLAGS` → `R11`。③ 清除 `RFLAGS` 中 `TF`/`IF` 以外的某些位（不关 IF）。④ `CS` ← `MSR_STAR[47:32]`（内核代码段）。⑤ `SS` ← `MSR_STAR[47:32]+8`（内核数据段）。⑥ `RIP` ← `MSR_LSTAR`（入口函数）。⑦ 切换到内核栈（`TSS.sp0`）。不压入错误码，不查 IDT。"),
            ("syscall 入口 `entry_SYSCALL_64` 做了哪些操作？", "① 切换到内核栈（`swapgs` 切换 GS 基址 + 读 `TSS.sp0`）。② 压入用户 `RIP`（`RCX`）、`RFLAGS`（`R11`）到内核栈（`pt_regs` 结构）。③ 压入其他寄存器（`pt_regs` 完整保存）。④ 检查 `syscall_nr` 范围。⑤ 调 `sys_call_table[syscall_nr]`。⑥ 返回时 `sysret` 指令恢复寄存器 + 切回用户栈 + `swapgs`。现代内核还加了 spectre/meltdown 缓解（KPTI 页表切换）。"),
            ("KPTI（Kernel Page Table Isolation）对 syscall 延迟有什么影响？", "KPTI 在每次 syscall/IRQ 进入内核时切换页表（用户态页表不映射内核地址），防 Meltdown 侧信道攻击。代价：① 每次 syscall 额外一次 `CR3` 写入 + TLB flush（部分）。② syscall 延迟增加 ~100-300ns。缓解：PCID (Process Context ID) 减少 TLB flush。`nopti` 启动参数可禁用（安全风险）。HFT 在受控环境可 `nopti` + `nospectre_v2` 换取性能。"),
        ],
    ),
    f"{CH10}/section-5-参数传递.md": (
        [
            "把 ULK 的参数传递（寄存器 + 栈）当现代版——x86-64 syscall 前 6 参数在 `rdi/rsi/rdx/r10/r8/r9`，无栈传参",
            "混淆 `r10` 和 `rcx`——syscall 用 `r10` 传第 4 参数（因为 `rcx` 被用来存返回地址）",
            "以为可以传任意多参数——Linux syscall 最多 6 个参数，超过的需要用结构体指针",
        ],
        [
            ("x86-64 syscall 的参数传递约定和函数调用约定有什么区别？", "函数调用（System V ABI）：参数在 `rdi/rsi/rdx/rcx/r8/r9`。syscall：参数在 `rdi/rsi/rdx/r10/r8/r9`——第 4 参数从 `rcx` 改为 `r10`，因为 `syscall` 指令用 `rcx` 保存返回地址。这就是为什么汇编 syscall 代码中常见 `mov %rcx, %r10`。超过 6 个参数的 syscall（如 `mmap` 有 6 个、`clone` 有 5 个）正好用满寄存器。"),
            ("用户态指针参数怎么安全传递到内核？", "不能直接解引用！① `access_ok(addr, size)`：检查地址在用户空间范围（防内核地址伪造）。② `copy_from_user(kbuf, ubuf, size)`：安全复制，如果 `ubuf` 无效则返回未复制的字节数。③ `get_user(x, ptr)`/`put_user(x, ptr)`：读/写简单类型（int/long/pointer）。④ `strncpy_from_user()`：安全复制字符串。这些函数都有 page fault fixup——用户指针触发 fault 时返回 `-EFAULT` 而非 panic。"),
            ("HFT 如何避免 syscall 参数传递的开销？", "① 共享内存 + 原子操作：数据通过 `mmap(MAP_SHARED)` 共享，用 `std::atomic` 同步，不需要 syscall 传参。② `io_uring` SQE：一次 `mmap` 映射 SQ/CQ ring，之后 submit I/O 只需写 ring + `io_uring_enter`（或甚至不调——SQPOLL 模式）。③ `vDSO`：`clock_gettime` 等直接读共享页，无参数传递开销。④ `seccomp-bpf` 缓存：同一 syscall 反复调用时跳过 seccomp 检查。"),
        ],
    ),
    f"{CH10}/section-6-参数验证与内核封装.md": (
        [
            "以为 `access_ok()` 就能保证指针安全——`access_ok()` 只检查地址范围，不保证页已映射或可写",
            "在内核中直接 `memcpy()` 用户指针——必须用 `copy_from_user()`，否则可能 panic 或安全漏洞",
            "忽略 `__user` 标注——`__user` 是 sparse 工具的标注，帮助发现未经验证的用户指针使用",
        ],
        [
            ("`access_ok()` 检查什么？不检查什么？", "检查：地址 + 大小在用户空间范围内（x86-64: `addr + size <= TASK_SIZE`，通常 0x7fffffffffff）。不检查：① 页是否已映射（可能仍触发 #PF）。② 页是否可写（`access_ok(VERIFY_WRITE, ...)` 已废弃，现代只检查范围）。③ 指针是否指向有效数据。`access_ok()` 是第一道防线，`copy_from_user()` 是真正的安全网（有 fault fixup）。"),
            ("`copy_from_user()` 为什么比 `memcpy()` 安全？", "① `access_ok()` 预检查。② 每次访问都注册在异常表中（`__ex_table`）。③ 如果用户页不可读（未映射/swap/权限不对），触发 #PF → `fixup_exception()` 找到 fixup 地址 → 跳到 `copy_from_user` 的错误返回点 → 返回未复制的字节数。`memcpy()` 直接解引用用户指针 → 如果页不可用 → 内核态 #PF → `die()`/panic。这就是为什么内核必须用 `copy_from_user`。"),
            ("HFT 中如何减少 `copy_from_user`/`copy_to_user` 的开销？", "① 共享内存：`mmap(MAP_SHARED)` 让用户态和内核态共享物理页，零拷贝。② `io_uring`：SQE/CQE 通过共享 ring 传递，`io_uring_enter` 只通知不拷贝。③ `splice`/`sendfile`：内核内数据搬运，不经过用户空间。④ `MSG_ZEROCOPY`：网络发送零拷贝（网卡 DMA 直接读用户页）。⑤ 大块数据：一次 `copy_from_user` 大块 > 多次小块（减少 `access_ok` 调用次数）。"),
        ],
    ),
}

# ============================================================
# Merge all notes
# ============================================================
ALL_NOTES = {}
ALL_NOTES.update(CH2_NOTES)
ALL_NOTES.update(CH3_NOTES)
ALL_NOTES.update(CH4_NOTES)
ALL_NOTES.update(CH5_NOTES)
ALL_NOTES.update(CH7_NOTES)
ALL_NOTES.update(CH8_NOTES)
ALL_NOTES.update(CH9_NOTES)
ALL_NOTES.update(CH10_NOTES)

# ============================================================
# Main
# ============================================================
ok = 0
miss = 0
skip = 0

for rel_path, (traps, quiz) in ALL_NOTES.items():
    fpath = os.path.join(BASE, rel_path)
    if not os.path.exists(fpath):
        print(f"MISSING: {rel_path}")
        miss += 1
        continue

    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    if "### 常见陷阱" in content:
        print(f"SKIP (already has): {rel_path}")
        skip += 1
        continue

    block = make_block(traps, quiz)
    new_content = insert_before_nav(content, block)

    with open(fpath, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"OK: {rel_path}")
    ok += 1

print(f"\n=== Done: {ok} OK, {miss} MISSING, {skip} SKIP ===")
