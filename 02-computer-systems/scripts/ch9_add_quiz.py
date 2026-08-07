#!/usr/bin/env python3
"""
Ch9 虚拟内存 — 新手化批量改造脚本
对 21 个 section 文件：替换空壳自测 → 常见陷阱(3) + 折叠自测题(4)
对内容过薄的文件：插入扩充内容
"""

import os

NOTES_DIR = os.path.join(os.path.dirname(__file__), "..", "chapter-09-virtual-memory", "notes")

# 空壳自测的匹配模式
SHELL_PATTERN = """### 口述巩固 · 自测

1. （待口述补）本节核心一句话？"""

# ─── 每个 section 的扩充内容 + 陷阱 + 自测题 ───

SECTIONS = [
    # ── 9.1 物理和虚拟寻址 (34行, 有内容) ──
    {
        "filename": "section-9.1-物理和虚拟寻址·名字辨析.md",
        "expand": None,
        "traps": [
            "**虚拟地址不是「假地址」** — 它是程序实际使用的真地址，MMU 负责翻译为 PA，程序本身无感知",
            "**MMU 翻译发生在 cache 访问之前** — PIPT cache 用 PA 索引，所以 VA→PA 必须先完成（TLB hit 时接近零开销）",
            "**swap 是 OS 层面策略，cache 是硬件层面机制** — 不要把缺页处理和 cache miss 混为一谈；缺页代价（μs 级）比 cache miss（ns 级）大 1000 倍以上",
        ],
        "quiz": [
            ("Q1: 程序指令里的指针是 VA 还是 PA？为什么程序不需要知道 PA？",
             "是 VA。MMU 在每次访存时自动完成 VA→PA 翻译，程序看到的是连续、整洁的虚拟地址空间，物理碎片由 OS 透明处理。"),
            ("Q2: 虚拟内存的四个作用分别是什么？",
             "1) 隔离 — 每进程独立 VA；2) 扩容 — 冷页 swap 到磁盘；3) 按需加载 — 只驻留用到的页；4) 简化编程 — 连续 VA，碎片透明。"),
            ("Q3: L1/L2/L3 cache 缓存的是 VA 还是 PA？",
             "PA（物理地址）。现代 x86 用 PIPT（物理索引物理标记），在 MMU 翻译后访问 cache，避免别名问题。"),
            ("Q4: 缺页（page fault）和 cache miss 的代价差多少量级？",
             "缺页 ≈ μs 级（可能触发磁盘 I/O），cache miss ≈ ns 级。差距约 1000–100000 倍。HFT 中缺页是灾难性事件。"),
        ],
    },

    # ── 9.2 地址空间 (19行, 有少量内容) ──
    {
        "filename": "section-9.2-地址空间.md",
        "expand": None,
        "traps": [
            "**48 位有效 VA 不等于 64 位** — x86-64 高 16 位是符号扩展（canonical address），非法地址触发 fault",
            "**VA 空间大小 ≠ 物理内存大小** — 8GB 物理机器的进程仍可拥有 128TB VA 空间，靠按需分配+swap 实现",
            "**同 VA 不同进程映射不同 PA** — 两个进程的 0x400000 完全独立，这是隔离的核心机制",
        ],
        "quiz": [
            ("Q1: x86-64 用户空间有效 VA 位数是多少？对应的地址空间大小？",
             "48 位有效（高 16 位符号扩展），用户空间 0x0000000000000000–0x00007FFFFFFFFFFF，约 256TB。"),
            ("Q2: 为什么物理内存只有 8GB，进程却可以「以为」拥有 256TB 地址空间？",
             "VA 空间是逻辑概念，只有被访问到的页才分配物理页帧（demand paging），冷页可 swap 到磁盘。VA→PA 映射由页表维护。"),
            ("Q3: 两个进程的虚拟地址 0x400000 指向同一物理地址吗？",
             "不指向。每进程有独立页表，同 VA 映射不同 PA。除非显式共享（如共享库、mmap MAP_SHARED），否则互不可见。"),
            ("Q4: canonical address 是什么？非法 canonical 地址会怎样？",
             "x86-64 要求 VA 高 16 位是低 48 位最高位的符号扩展。不满足此格式的地址触发 #GP（通用保护异常）。"),
        ],
    },

    # ── 9.3 VM as cache (17行, 近空) ──
    {
        "filename": "section-9.3-虚拟内存作为缓存工具.md",
        "expand": """### 虚拟内存 = DRAM 缓存磁盘

- **概念：** VM 将 DRAM 视为磁盘（swap）的缓存，以 **页（4KB）** 为单位
- **页表（PTE）= 缓存标记** — 记录页是否驻留（valid bit）、位置、权限
- **命中（page hit）** — 页在 DRAM，直接访问
- **缺页（page fault）** — 页不在 DRAM，OS 从磁盘装入：
  1. 触发异常 → OS 接管
  2. 选牺牲页（LRU/近似）→ 若脏则写回磁盘
  3. 从磁盘读入新页 → 更新 PTE
  4. 重启触发指令

| 概念 | VM 缓存 | CPU cache |
|------|---------|-----------|
| 缓存粒度 | 页 4KB | 行 64B |
| 缓存介质 | DRAM 缓存磁盘 | SRAM 缓存 DRAM |
| miss 代价 | μs 级 | ns 级 |
| 替换策略 | OS 软件 LRU | 硬件近似 LRU |

**局部性保证可行性：** 工作集 ≪ VA 空间，多数页无需驻留。

""",
        "traps": [
            "**VM 缓存粒度是页（4KB），CPU cache 粒度是行（64B）** — 不要混淆两个层次的缓存",
            "**缺页代价远大于 cache miss** — 缺页可能触发磁盘 I/O（μs 级），cache miss 只等 SRAM（ns 级）",
            "**demand paging ≠ prefetch** — 默认按需加载，不预读；OS 可预读相邻页但不是必须",
        ],
        "quiz": [
            ("Q1: 虚拟内存作为缓存工具时，缓的是什么？被缓的是什么？",
             "DRAM 缓存磁盘（swap 空间）。热页在 DRAM，冷页在磁盘，以 4KB 页为传输单位。"),
            ("Q2: page hit 和 page fault 分别是什么？",
             "page hit = 访问的页已在 DRAM，PTE valid bit=1。page fault = 页不在 DRAM，触发异常由 OS 从磁盘装入。"),
            ("Q3: 为什么虚拟内存方案可行？基础前提是什么？",
             "局部性原理（spatial + temporal）。程序工作集远小于 VA 空间，大部分页不需要同时驻留。"),
            ("Q4: 缺页处理时 OS 做哪些步骤？",
             "1) 触发异常；2) 选牺牲页（若脏先写回）；3) 从磁盘读入新页；4) 更新 PTE；5) 重启触发指令。"),
        ],
    },

    # ── 9.4 VM as memory management (19行, 有少量内容) ──
    {
        "filename": "section-9.4-虚拟内存作为内存管理工具.md",
        "expand": None,
        "traps": [
            "**简化链接 ≠ 链接器不管地址** — 链接器仍处理符号解析和重定位，只是 VA 布局可预测（代码段总从固定地址开始）",
            "**共享页靠映射同一 PA，不是拷贝** — 多个进程的 PTE 指向同一物理页帧，OS 标记只读，写时才 COW",
            "**COW 在 fork 时不复制，写时才复制** — fork 只复制页表，共享只读页；首次写触发 COW 才分配新物理页",
        ],
        "quiz": [
            ("Q1: 虚拟内存如何简化链接？",
             "每个程序可假设相同的 VA 布局（代码段从固定地址开始、栈在高端），链接器不需要为每个程序生成不同的地址方案。"),
            ("Q2: execve 加载新程序时，如何处理物理内存？",
             "不必将整个程序塞入连续物理内存。execve 只创建 VA→文件的映射（mmap ELF LOAD 段），按需 fault-in 物理页。"),
            ("Q3: 多进程如何共享 libc 代码？",
             "各进程 PTE 映射到同一物理页帧，标记为只读。因为代码段不可写，无需 COW，直接共享物理页。"),
            ("Q4: fork 后父子进程的页表关系是什么？何时才真正分配新物理页？",
             "fork 复制页表（不是复制页），父子共享物理页（只读）。任一方写时触发 COW，才分配新物理页。"),
        ],
    },

    # ── 9.5 VM as protection (17行, 近空) ──
    {
        "filename": "section-9.5-虚拟内存作为保护工具.md",
        "expand": """### PTE 权限位与保护机制

- **PTE 权限位：** `SUP`（内核专用）/ `R` / `W` / `X`
- **检查时机：** 每次 MMU 地址翻译时自动检查，无需软件参与
- **违规 → page fault** — CPU 触发异常，OS 终止进程或发送信号

| 权限位 | 含义 | 违规后果 |
|--------|------|----------|
| SUP=1 | 仅内核可访问 | 用户态访问 → SIGSEGV |
| R=0 | 不可读 | 读操作 → SIGSEGV |
| W=0 | 不可写 | 写操作 → SIGSEGV（含 COW 触发） |
| X=0 | 不可执行 | 执行 → SIGSEGV（DEP/NX） |

**HFT：** 代码段设 R+X，数据段设 R+W（不执行），栈不可执行（防溢出利用）。

""",
        "traps": [
            "**保护检查是 PTE 硬件位，不是软件检查** — MMU 在翻译时自动验证，不需要 OS 逐次审查",
            "**用户态访问内核页 → SIGSEGV，不是 silently fail** — SUP 位违规直接触发异常终止进程",
            "**权限粒度是页级（4KB），不是字节级** — 同一页内所有字节权限相同；细粒度保护需要特殊机制（MPK 等）",
        ],
        "quiz": [
            ("Q1: PTE 的 SUP 位是什么作用？",
             "SUP=1 表示该页仅内核态可访问。用户态访问 SUP=1 的页会触发 page fault → SIGSEGV。"),
            ("Q2: DEP（数据执行保护）依赖哪个 PTE 位？",
             "X（可执行）位。数据页设 X=0，代码页设 X=1。尝试执行数据页（如栈溢出 shellcode）触发 SIGSEGV。"),
            ("Q3: 为什么写只读页（W=0）有时不报错而是触发 COW？",
             "fork 后共享页标记为只读。写时 MMU 检测到 W=0 触发 fault，OS 判断是 COW 场景，分配新物理页后重启写操作，而非终止进程。"),
            ("Q4: 内存保护粒度是什么？能否实现字节级保护？",
             "页级（4KB），同一页所有字节权限相同。字节级保护需要硬件扩展（如 Intel MPK 提供 16 个 protection key），但仍有页级限制。"),
        ],
    },

    # ── 9.6 地址翻译 (41行, 有内容) ──
    {
        "filename": "section-9.6-地址翻译.md",
        "expand": None,
        "traps": [
            "**TLB miss ≠ page fault** — TLB miss 只是缓存未命中，MMU 去 walk 页表找 PTE；page fault 是 PTE 本身标记页不在内存",
            "**多级页表省空间但增加访存次数** — 每级页表需一次内存访问，4 级 = 4 次访存（TLB miss 时）；TLB 命中则只需 1 次",
            "**大页减 TLB 压力但增内部碎片** — 2MB 大页覆盖更多 VA，但分配后未用部分浪费；HFT 用大页覆盖热数据区域",
        ],
        "quiz": [
            ("Q1: VA 如何划分为 VPN 和 VPO？4KB 页时各多少位？",
             "VPO = 页内偏移（4KB → 12 位），VPN = 剩余位。x86-64 48 位有效 VA：VPO 12 位 + VPN 36 位。"),
            ("Q2: TLB miss 时 MMU 做什么？和 page fault 有何区别？",
             "TLB miss → MMU walk 页表（多级），从内存读 PTE 装入 TLB，然后重试翻译。page fault 是 PTE 显示页不在 DRAM，需 OS 介入从磁盘装入。TLB miss 是硬件处理，page fault 是软件处理。"),
            ("Q3: 4 级页表如何节省内存？单级页表有什么问题？",
             "单级页表：48 位 VA / 4KB 页 = 2^36 个 PTE × 8B ≈ 512GB/进程，不可行。4 级页表只分配用到的区域，未用区域不占物理页。"),
            ("Q4: HFT 为什么用大页（2MB/1GB）？具体好处是什么？",
             "同样工作集用更少 TLB 项覆盖。2MB 页 vs 4KB 页 → TLB 项减少 512 倍，减少 TLB miss 导致的页表 walk（每次 walk 数十周期），降低尾延迟。"),
        ],
    },

    # ── 9.7 i7/Linux 案例 (17行, 近空) ──
    {
        "filename": "section-9.7-IntelCorei7-Linux案例.md",
        "expand": """### 9.7.1 Core i7 页表结构

- **4 级页表：** CR3 → PML4 → PDPT → PD → PT → 物理页
- **每级 9 位索引**（512 项），12 位页偏移 → 48 位 VA
- **PTE 格式：** P（present）、R/W、U/S、WT、NC、A（accessed）、D（dirty）、PFN

### 9.7.2 Linux 页表管理

- **Linux 抽象层：** `pgd` → `pud` → `pmd` → `pte`，适配不同架构
- **TLB 结构（i7 典型）：** L1 dTLB（64 项 4KB + 32 项 2MB）、L2 STLB（统一 1536 项）
- **TLB 刷新：** `invlpg` 单页刷新；`cr3` 写入全刷；PCID 减少上下文切换 TLB 失效

**HFT：** 上下文切换后 TLB 冷 → 首批指令慢；用 CPU 绑定 + 大页减少 TLB miss。

""",
        "traps": [
            "**i7 用 4 级页表（48 位 VA），Linux 5.x+ 可选 5 级（57 位 VA）** — 不要假设永远是 4 级，检查 `/proc/cpuinfo` 的 `la57`",
            "**TLB 是每核私有的，不是共享的** — 上下文切换可能刷 TLB（无 PCID 时），这是切换后首批访存慢的原因",
            "**Linux 页表存在内核空间，用户进程不可直接读** — `/proc/self/pagemap` 需 root 或 `CAP_SYS_ADMIN`",
        ],
        "quiz": [
            ("Q1: Core i7 的 4 级页表分别叫什么？CR3 寄存器存什么？",
             "PML4 → PDPT → PD → PT。CR3 存 PML4 表的物理地址（顶层页表基址）。"),
            ("Q2: x86-64 48 位 VA 如何分配给 4 级页表索引和页偏移？",
             "12 位页偏移 + 4 × 9 位索引 = 48 位。每级 9 位 = 512 项，每项 8B → 每级页表恰好 4KB（一页）。"),
            ("Q3: 上下文切换时 TLB 会怎样？PCID 如何帮助？",
             "无 PCID 时切换需刷 TLB（写 CR3），新进程首批访存全部 TLB miss。PCID 给每进程分配 ID，切换时可不刷 TLB，减少切换开销。"),
            ("Q4: HFT 中如何减少 TLB miss？",
             "1) CPU 绑定减少上下文切换；2) 大页（2MB/1GB）减少 TLB 项数；3) `mlock` 防止页被换出；4) 预 fault 热数据区域。"),
        ],
    },

    # ── 9.8 mmap (45行, 有内容) ──
    {
        "filename": "section-9.8-内存映射mmap.md",
        "expand": None,
        "traps": [
            "**mmap 不立即分配物理页** — 只创建 VA→文件的映射，首次访问时才 page fault 分配物理页（lazy allocation）",
            "**MAP_PRIVATE 写触发 COW，不是共享** — 写操作复制新物理页，原文件不变；MAP_SHARED 才真正共享写入",
            "**munmap 不保证 flush** — 已写脏页由内核异步写回；需 `msync(MS_SYNC)` 强制同步",
        ],
        "quiz": [
            ("Q1: MAP_SHARED 和 MAP_PRIVATE 的核心区别？",
             "MAP_SHARED：多进程映射同一物理页，写操作互相可见且（映射文件时）写回文件。MAP_PRIVATE：写时复制，各进程独立副本，互不影响，原文件不变。"),
            ("Q2: fork 后子进程的 mmap 区域如何继承？",
             "fork 复制页表。MAP_SHARED 区域父子共享同一物理页（写入互相可见）；MAP_PRIVATE 区域标记只读，写时 COW 分配新页。"),
            ("Q3: HFT 用 mmap 做行情文件 replay 有什么好处？需要注意什么？",
             "好处：1) 避免 read() 系统调用开销；2) 内核页缓存自动管理；3) 顺序访问局部性好。注意：首次访问 page fault 有延迟，应用 MAP_POPULATE 预 fault 或手动预读。"),
            ("Q4: DPDK 为什么用 hugetlbfs + mmap 大页？",
             "大页（2MB/1GB）减少 TLB 项数，避免页表 walk 延迟；hugetlbfs 预留大页池，保证分配不失败；mmap 直接映射到用户空间，零拷贝访问网卡 DMA 缓冲。"),
        ],
    },

    # ── 9.9.1 malloc/free (24行, 有内容) ──
    {
        "filename": "section-9.9.1-malloc和free.md",
        "expand": None,
        "traps": [
            "**malloc 返回未初始化内存，calloc 清零** — 不要假设 malloc 的内存是 0，否则引入难排查的 bug",
            "**free 后指针不变，悬空引用（UAF）风险** — free 只标记块为空闲，不清零指针；建议 free 后立即置 NULL",
            "**sbrk/brk 是系统调用，malloc 是库函数** — malloc 在用户态管理堆，只有堆不够时才 sbrk 向内核要更多页",
        ],
        "quiz": [
            ("Q1: malloc、calloc、realloc 的区别？",
             "malloc(size) 分配未初始化内存；calloc(n, size) 分配并清零；realloc(ptr, size) 调整大小，可能移动内存（返回新指针，旧指针失效）。"),
            ("Q2: free 一个指针后，指针本身的值变了吗？为什么有 UAF 风险？",
             "不变。free 只标记该块为空闲，不清零指针。如果继续使用该指针（UAF），可能读到被重新分配的数据或触发 crash。"),
            ("Q3: malloc 和 sbrk 的关系是什么？",
             "malloc 是用户态库函数，管理堆的空闲链表。sbrk/brk 是系统调用，调整 program break。malloc 只在现有堆不够时才调 sbrk 向内核申请新页。"),
            ("Q4: HFT 中为什么避免在热路径调用 malloc？",
             "malloc 耗时不确定（可能触发 sbrk 系统调用 + 页表操作），且可能引起碎片。HFT 用预分配池/对象池，热路径只从池中取，零系统调用。"),
        ],
    },

    # ── 9.9.2 why dynamic alloc (17行, 近空) ──
    {
        "filename": "section-9.9.2-为何动态分配.md",
        "expand": """### 为什么需要动态分配

- **运行时才知道大小** — 变长数组、动态数据结构
- **生命周期不固定** — 栈变量随函数返回释放，堆可跨函数/线程存活
- **数据结构动态增长** — 链表、树、哈希表节点按需分配

| 分配方式 | 生命周期 | 速度 | 灵活性 |
|----------|----------|------|--------|
| 栈（局部变量） | 函数返回自动释放 | 最快（移动 SP） | 固定大小 |
| 静态/全局 | 程序整个生命周期 | 快 | 固定大小 |
| 堆（malloc） | 手动 free | 慢（库+可能 syscall） | 任意大小 |

**HFT：** 热路径用栈/预分配池；启动阶段用堆分配大缓冲，之后不再 malloc。

""",
        "traps": [
            "**栈分配自动释放，堆需手动 free** — 忘记 free → 内存泄漏；free 后继续用 → UAF",
            "**动态分配 ≠ 慢** — 池化后接近栈速度；慢的是碎片化和系统调用",
            "**realloc 可能移动内存，旧指针失效** — 不要假设 realloc 原地扩展",
        ],
        "quiz": [
            ("Q1: 栈分配和堆分配的主要区别？",
             "栈：移动 SP 指针，函数返回自动释放，速度最快但大小固定。堆：通过 malloc/free 手动管理，大小灵活但需维护空闲链表，可能触发系统调用。"),
            ("Q2: 为什么 HFT 热路径避免动态分配？用什么替代？",
             "动态分配耗时不确定（可能 sbrk + 页表操作）。替代：启动时预分配对象池/环形缓冲，热路径从池中取/还，零系统调用。"),
            ("Q3: realloc 返回值和原指针一定相同吗？",
             "不一定。如果原地扩展不够，realloc 分配新块、拷贝数据、释放旧块，返回新指针。使用旧指针是 UAF。"),
            ("Q4: 哪些场景必须用动态分配而不是栈？",
             "1) 大小运行时才确定；2) 生命周期超出函数范围（如跨线程传递）；3) 大数组（栈空间有限，默认 8MB）；4) 动态数据结构节点。"),
        ],
    },

    # ── 9.9.3 allocator goals (21行, 有表格) ──
    {
        "filename": "section-9.9.3-分配器目标.md",
        "expand": None,
        "traps": [
            "**吞吐和利用率是 tradeoff** — 首次适配吞吐高但碎片多；最佳适配利用率高但搜索慢",
            "**O(1) 不代表快** — 常数可能很大（如分离链表的桶查找）",
            "**空间利用率 ≠ 无碎片** — 利用率高但碎片可能也很高（大量小碎片）",
        ],
        "quiz": [
            ("Q1: 分配器的两个核心目标是什么？它们矛盾吗？",
             "吞吐量（malloc/free 速度）和空间利用率（少碎片）。矛盾：追求吞吐（首次适配）碎片多；追求利用率（最佳适配）搜索慢。"),
            ("Q2: 吞吐量和利用率能否同时最优？",
             "理论上不能。实际工程用分离适配（segregated fit）在两者间取平衡：按大小分桶，桶内首次适配，兼顾速度和利用率。glibc ptmalloc 就是这种策略。"),
            ("Q3: 为什么说「O(1) 不代表快」？",
             "大 O 表示法忽略常数。分离链表是 O(1) 但需要计算桶号、操作链表指针、可能加锁；实际耗时可能比简单首次适配的 O(n)（n 很小时）还慢。"),
            ("Q4: HFT 场景下哪个目标更重要？",
             "吞吐量（确定性延迟）远比利用率重要。HFT 用固定大小对象池，O(1) 且无碎片，牺牲灵活性换确定性。"),
        ],
    },

    # ── 9.9.4 fragmentation (18行, 有内容) ──
    {
        "filename": "section-9.9.4-碎片.md",
        "expand": None,
        "traps": [
            "**内部碎片 = 块内浪费，外部碎片 = 块间间隙** — 分配 10B 实际给 16B → 内部 6B；空闲总和够但不连续 → 外部",
            "**外部碎片更难处理** — 内部碎片靠减小对齐粒度；外部碎片靠合并/压缩（compaction，C 不支持因为有指针）",
            "**对齐引起的内部碎片不可避免** — 16 字节对齐时分配 1B 也占 16B，这是硬件/ABI 要求",
        ],
        "quiz": [
            ("Q1: 内部碎片和外部碎片的定义分别是什么？",
             "内部碎片：分配块 > 请求大小，块内多余空间浪费。外部碎片：空闲块总和够大但不连续，无法满足大请求。"),
            ("Q2: 16 字节对齐时分配 1 字节，浪费多少？属于哪种碎片？",
             "实际分配 16 字节，浪费 15 字节。属于内部碎片（块内浪费）。"),
            ("Q3: 为什么 C 程序的外部碎片不能像 Java 一样用 compaction 解决？",
             "Compaction 需要移动对象并更新所有指向它的指针。C/C++ 有裸指针，GC 无法找到所有引用。Java 有引用跟踪，可以移动对象。"),
            ("Q4: HFT 如何避免外部碎片？",
             "1) 固定大小对象池（无分割）；2) 启动时预分配大块，不再 malloc/free；3) 环形缓冲替代动态数组；4) 避免长生命周期和短生命周期混合分配。"),
        ],
    },

    # ── 9.9.5-9.9.6 impl & implicit free list (17行, 近空) ──
    {
        "filename": "section-9.9.5-9.9.6-实现问题与隐式空闲链表.md",
        "expand": """### 隐式空闲链表

- **块格式：** `[header(4-8B)] [payload] [padding] [footer(4-8B)]`
- **header/footer 存：** 块大小 + 分配位（1=已分配，0=空闲）
- **隐式：** 所有块（已分配+空闲）通过 size 隐含链接，遍历时用 `当前地址 + size` 跳到下一块
- **操作：**
  - `find_fit` — 从头遍历找够大的空闲块
  - `split` — 空闲块大于需求时切分
  - `coalesce` — free 时合并相邻空闲块

```
堆布局：[prologue][block1][block2]...[epilogue]
        ← header ← payload → ← footer →
```

**问题：** 遍历所有块（含已分配）→ O(n)，n = 总块数。

""",
        "traps": [
            "**隐式链表遍历所有块（含已分配），慢** — 查找空闲块要跳过所有已分配块，O(n)",
            "**header/footer 占额外空间（8-16B/块）** — 小块时开销比例大（16B 块 → 50%+ 开销）",
            "**对齐要求决定最小块大小** — 16B 对齐 → header(4)+payload(1)+pad+footer(4) → 最小 16B",
        ],
        "quiz": [
            ("Q1: 隐式空闲链表的「隐式」是什么意思？",
             "不需要显式指针链接空闲块。所有块（已分配+空闲）按地址连续排列，通过 header 中的 size 字段隐含「下一块」的位置。"),
            ("Q2: header 和 footer 各存什么？为什么需要 footer？",
             "都存块大小+分配位。footer 用于从当前块地址反向定位前一个块（coalesce 时判断前块是否空闲）。"),
            ("Q3: 隐式链表查找空闲块的时间复杂度？为什么慢？",
             "O(n)，n=总块数（含已分配）。因为要遍历所有块跳过已分配的，即使大部分已分配也要逐个检查。"),
            ("Q4: 如果 16 字节对齐，header 4B + footer 4B，最小块多大？",
             "至少 16B：header(4) + payload(至少 1B) + padding(7B) + footer(4B) → 向上对齐到 16B。这意味着分配 1B 实际占 16B。"),
        ],
    },

    # ── 9.9.7-9.9.9 placement/split/grow (19行, 有内容) ──
    {
        "filename": "section-9.9.7-9.9.9-放置、分割、扩展堆.md",
        "expand": None,
        "traps": [
            "**首次适配快但碎片多，最佳适配慢但碎片少** — 下一次适配折中但可能跳过合适块",
            "**分割要留最小块大小** — 切剩的碎片如果 < 最小块（16B），不如不分（全部给请求者）",
            "**sbrk 扩展是系统调用** — 应批量申请（如一次 4KB-64KB），而不是每个 malloc 都 sbrk",
        ],
        "quiz": [
            ("Q1: 首次适配、下一次适配、最佳适配的区别？",
             "首次适配：从头找第一个够大的。下一次适配：从上次位置继续找（快但碎片多）。最佳适配：找最小的够大块（利用率高但搜索全表）。"),
            ("Q2: 什么时候不该分割空闲块？",
             "分割后剩余部分 < 最小块大小（如 16B）时。碎片太小无法满足任何请求，反而增加 header/footer 开销。此时应把整块给请求者。"),
            ("Q3: 堆扩展（sbrk）为什么不是每次 malloc 都调？",
             "sbrk 是系统调用，开销大（上下文切换+页表操作）。malloc 库维护空闲池，只在池不够时批量 sbrk（如一次申请 64KB），分摊系统调用开销。"),
            ("Q4: HFT 如何避免堆扩展的不确定延迟？",
             "启动时预分配足够大的堆（一次 sbrk/mmap），之后不再扩展。热路径从预分配池中分配，零系统调用。"),
        ],
    },

    # ── 9.9.10-9.9.11 coalescing & boundary tags (18行, 有内容) ──
    {
        "filename": "section-9.9.10-9.9.11-合并与边界标记.md",
        "expand": None,
        "traps": [
            "**边界标记用 footer 判断前块状态，但占空间** — 每块多 4-8B footer；可优化为只在空闲块存 footer",
            "**合并只合空闲块，已分配不动** — free 后检查前后块，只有空闲的才合并",
            "**合并方向：前后都看** — 只看后块会漏掉与前块合并的机会；前块用 footer 定位",
        ],
        "quiz": [
            ("Q1: 为什么 free 时要合并相邻空闲块？",
             "防止外部碎片累积。如果不合并，多次 free 后产生大量小碎片，总和够大但不连续，无法满足大请求。"),
            ("Q2: 边界标记（boundary tag）是什么？解决什么问题？",
             "每个块尾部存一个 footer，内容和 header 一样（size + alloc bit）。解决「如何 O(1) 找到前一块」的问题：当前块地址 - 前一块 footer 中的 size = 前一块起始地址。"),
            ("Q3: 边界标记的代价是什么？如何优化？",
             "代价：每块多 4-8B（footer），小块开销比例大。优化：只在空闲块存 footer，已分配块不存（因为已分配块不会被合并，不需要查前块状态）。"),
            ("Q4: 合并时需要检查几个方向？为什么？",
             "两个方向：前一块和后一块。只合并后块会漏掉与前块合并的机会，导致碎片。前块通过 footer 的 size 字段定位，后块通过当前块 header 的 size 跳转。"),
        ],
    },

    # ── 9.9.12 simple allocator (17行, 近空) ──
    {
        "filename": "section-9.9.12-简单分配器综合.md",
        "expand": """### CSAPP Malloc Lab 概述

- **目标：** 实现 `mm_init` / `mm_malloc` / `mm_free` / `mm_realloc`
- **约束：** 只能调 `mem_sbrk` 扩展堆，不能直接用系统 malloc
- **评分：** 空间利用率 × 吞吐量

| 函数 | 职责 |
|------|------|
| `mm_init` | 初始化堆：prologue + epilogue 块 |
| `mm_malloc` | find_fit → split → 返回 payload 指针 |
| `mm_free` | 标记空闲 → coalesce 前后 |
| `mm_realloc` | 特殊情况优化（原地扩展/缩小），否则 malloc+copy+free |

**优化路径：** 隐式链表 → 显式链表 → 分离链表 → 分离适配（最佳）

""",
        "traps": [
            "**Lab 的 mm_malloc 不等于 glibc malloc** — Lab 是教学简化版，glibc 用更复杂的 ptmalloc/tcmalloc/jemalloc",
            "**realloc 不一定原地扩展** — 可能 malloc 新块 + memcpy + free 旧块，数据量大时开销显著",
            "**测试用 trace 文件验证利用率和吞吐** — 利用率 = peak_payload / peak_heap，不只是不泄漏",
        ],
        "quiz": [
            ("Q1: CSAPP Malloc Lab 需要实现哪些函数？评分标准是什么？",
             "实现 mm_init/mm_malloc/mm_free/mm_realloc。评分 = 空间利用率（peak_payload / peak_heap）× 吞吐量（ops/sec）。"),
            ("Q2: mm_init 需要做什么？prologue 和 epilogue 块的作用？",
             "mm_init 调 mem_sbrk 创建初始堆结构。prologue 块（已分配，永久存在）防止合并越界到堆前；epilogue 块（size=0, alloc=1）标记堆尾，扩展堆时移动 epilogue。"),
            ("Q3: realloc 的优化策略有哪些？",
             "1) 缩小：原地改 header，分割尾部；2) 下一块空闲且够大：合并下一块，原地扩展；3) 否则：malloc 新块 + memcpy + free 旧块。"),
            ("Q4: 从隐式链表到分离适配，性能提升的路径是什么？",
             "隐式链表 O(n) → 显式链表 O(空闲块数) → 分离链表（按大小分桶）O(桶内空闲块数) → 分离适配（桶内首次适配）≈O(1)。"),
        ],
    },

    # ── 9.9.13 explicit free list (17行, 近空) ──
    {
        "filename": "section-9.9.13-显式空闲链表.md",
        "expand": """### 显式空闲链表

- **核心思想：** 空闲块内存 prev/next 指针，串成双向链表
- **优势：** find_fit 只遍历空闲块，跳过已分配块 → 快很多
- **结构：**
  ```
  空闲块：[header][prev][next][...padding...][footer]
  已分配块：[header][payload][footer]  （无 prev/next，省空间）
  ```
- **插入策略：**
  - **LIFO** — 新释放块插表头，O(1) 插入，但碎片多
  - **FIFO** — 插表尾，碎片分布均匀

| 对比 | 隐式链表 | 显式链表 |
|------|----------|----------|
| find_fit | O(总块数) | O(空闲块数) |
| 空闲块开销 | header+footer | +prev+next（8-16B） |
| 已分配块开销 | header+footer | header+footer（不变） |

""",
        "traps": [
            "**显式链表只在空闲块存指针，已分配块不占额外空间** — 指针存在空闲块的 payload 区域",
            "**LIFO vs FIFO 插入策略影响碎片分布** — LIFO 快但碎片集中在表头；FIFO 均匀但插入慢",
            "**指针占用空闲块的有效载荷空间** — 最小空闲块要容纳 header+prev+next+footer",
        ],
        "quiz": [
            ("Q1: 显式空闲链表和隐式链表的核心区别？",
             "显式链表在空闲块的 payload 中存 prev/next 指针，将所有空闲块串成双向链表。find_fit 只遍历空闲块，跳过已分配块，速度从 O(总块数) 提升到 O(空闲块数)。"),
            ("Q2: 已分配块需要存 prev/next 指针吗？为什么？",
             "不需要。已分配块不会被 find_fit 遍历，不需要链表指针。空闲时才在 payload 区域写入 prev/next，分配时这些空间归还给用户。"),
            ("Q3: LIFO 和 FIFO 插入策略各自的优缺点？",
             "LIFO：释放时插表头，O(1)，但反复分配释放同大小块会在表头形成碎片聚集。FIFO：插表尾，碎片分布均匀，但需要遍历到表尾（或维护尾指针）。"),
            ("Q4: 显式链表的最小空闲块大小由什么决定？",
             "必须容纳 header + prev 指针 + next 指针 + footer。16B 对齐时，最小空闲块通常 16-24B，比隐式链表的最小块大。"),
        ],
    },

    # ── 9.9.14 segregated free list (17行, 近空) ──
    {
        "filename": "section-9.9.14-分离空闲链表.md",
        "expand": """### 分离空闲链表（Segregated Free Lists）

- **核心：** 按块大小分类，每类一个空闲链表
- **查找：** 根据请求大小定位对应大小类的链表 → 只在该链表内搜索

**大小类划分示例：**
| 链表 | 大小范围 |
|------|----------|
| 0 | 1–16B |
| 1 | 17–32B |
| 2 | 33–64B |
| ... | 2 的幂 |
| k | > 2^(k+3) |

- **glibc ptmalloc：** 用分离适配（segregated fit），每个大小类一条链表
- **tcmalloc/jemalloc：** 更细粒度 + 线程局部缓存（thread-local cache）

**优势：** find_fit ≈ O(1)（直接定位大小类 + 链表短）

""",
        "traps": [
            "**分离链表 = 多个链表按大小分类，不是一条链** — 每个大小类一个独立链表",
            "**桶数选择是 tradeoff** — 多桶查找快但管理开销大（每个桶维护 prev/next）；少桶省空间但桶内链表长",
            "**glibc 的 ptmalloc 用分离适配** — 但不是最优；tcmalloc/jemalloc 用线程局部缓存进一步减少锁竞争",
        ],
        "quiz": [
            ("Q1: 分离空闲链表如何提升查找效率？",
             "按块大小分桶，每个大小类一条链表。malloc 时根据请求大小直接定位对应桶，只在该桶内搜索。桶内链表短，查找接近 O(1)。"),
            ("Q2: 大小类（size class）如何划分？",
             "通常按 2 的幂分组：1-16、17-32、33-64、65-128... 每个组对应一条链表。大请求搜更大的桶，小请求搜更小的桶。"),
            ("Q3: glibc malloc 用什么策略？和 tcmalloc/jemalloc 有何区别？",
             "glibc ptmalloc 用分离适配（多大小类链表）。tcmalloc/jemalloc 在此基础上加线程局部缓存（thread-local cache），减少锁竞争，多线程性能更好。"),
            ("Q4: HFT 应该用哪个分配器？为什么？",
             "最佳方案：自己实现固定大小对象池（无锁、无碎片、O(1)）。如果用通用分配器：jemalloc（线程局部缓存减少锁竞争，低尾延迟）。绝不在线程间共享一个 malloc arena。"),
        ],
    },

    # ── 9.10 GC (24行, 有内容) ──
    {
        "filename": "section-9.10-垃圾收集.md",
        "expand": None,
        "traps": [
            "**保守 GC 把像指针的位模式都当根，可能误留** — 整数碰巧像地址 → 该回收的块不回收（false positive，不误删）",
            "**stop-the-world 停顿对 HFT 致命** — GC 暂停所有应用线程，可能 ms-s 级；tick 线程必须避免",
            "**C/C++ 几乎不用 GC，用 RAII/池/Rust 所有权** — GC 的不确定性延迟与 HFT 的确定性要求矛盾",
        ],
        "quiz": [
            ("Q1: Mark & Sweep 的两个阶段分别做什么？",
             "Mark：从根（栈、全局变量、寄存器）出发沿指针图遍历，标记所有可达对象。Sweep：扫描整个堆，未标记的块回收加入空闲链表。"),
            ("Q2: C 的保守 GC（conservative GC）为什么不精确？",
             "C 没有类型信息，GC 无法区分指针和整数。把栈/寄存器中所有「像指针的位模式」都当根，可能误留不可达对象（但不误删可达对象）。"),
            ("Q3: 为什么 HFT 不用 GC？用什么替代？",
             "GC 的 stop-the-world 停顿不确定（ms-s 级），与 HFT 确定性延迟要求矛盾。替代：RAII（C++ 析构）、对象池（预分配）、Rust 所有权（编译期保证无泄漏/无 UAF）。"),
            ("Q4: GC 的延迟（latency tail）问题如何影响系统设计？",
             "即使平均 GC 停顿很短（μs），最坏情况可能很长（s）。系统设计要考虑最坏延迟：tick 线程不分配堆、用 pre-allocated buffer、GC 语言（Java/Go）需调优 GC 参数减少 STW 时间。"),
        ],
    },

    # ── 9.11 memory errors (34行, 有内容) ──
    {
        "filename": "section-9.11-C程序常见内存错误.md",
        "expand": None,
        "traps": [
            "**ASan 有性能开销（2-5x），生产不能开** — ASan 在每次访存检查影子内存，开发/CI 开，生产关",
            "**Valgrind 慢 10-50x，只用于测试** — 它模拟 CPU 执行，不修改二进制；ASan 编译期插桩",
            "**UAF 比泄漏更危险** — 泄漏只是浪费内存；UAF 可能被利用（攻击者控制释放后的块内容）",
        ],
        "quiz": [
            ("Q1: ASan 和 Valgrind 的工作原理和性能开销有什么区别？",
             "ASan：编译期插桩（-fsanitize=address），每次访存查影子内存，开销 2-5x，检测越界/UAF。Valgrind：运行期模拟 CPU 执行，不开编译选项，开销 10-50x，检测更全面（含未初始化读）。"),
            ("Q2: 为什么 UAF（use-after-free）比内存泄漏更危险？",
             "泄漏只是浪费内存，程序仍正确。UAF 访问已释放的内存，可能读到被重新分配的数据（逻辑错误），或被攻击者利用（控制释放块内容实现代码执行）。"),
            ("Q3: HFT 在 CI 和生产中分别用什么工具检查内存错误？",
             "CI：ASan + UBSan 编译测试二进制，Valgrind 跑集成测试。生产：不开 ASan（性能开销），靠代码规范 + 对象池 + RAII + Rust 策略层。"),
            ("Q4: off-by-one 错误如何防范？",
             "1) 循环用 `< n` 不是 `<= n`；2) 分配 `n+1` 字节给 n 字符串（留 '\\0'）；3) 边界检查用 `fgets` 不用 `gets`；4) ASan 可检测栈/堆越界。"),
        ],
    },

    # ── 9.12 summary (17行, 近空) ──
    {
        "filename": "section-9.12-小结.md",
        "expand": """### Ch9 全章要点

| 主题 | 核心概念 | HFT 关联 |
|------|----------|----------|
| §9.1-9.2 | VA/PA、地址空间 | 每进程独立 VA，隔离 |
| §9.3 | VM = DRAM 缓存磁盘 | 缺页代价 μs 级，避免 |
| §9.4-9.5 | VM 简化管理 + 保护 | 共享页、PTE 权限 |
| §9.6 | 地址翻译、TLB、多级页表 | 大页减 TLB miss |
| §9.7 | i7/Linux 4 级页表 | 上下文切换刷 TLB |
| §9.8 | mmap、COW | 行情文件 mmap、大页 |
| §9.9 | malloc/free、分配器 | 对象池替代 malloc |
| §9.10-9.11 | GC、内存错误 | ASan/Valgrind、RAII |

**一句话：** 虚拟内存 = 缓存（DRAM 缓存磁盘）+ 管理（隔离/共享/简化链接）+ 保护（PTE 权限位），地址翻译由 MMU+TLB+页表完成，malloc 在用户态管理堆。

""",
        "traps": [
            "**VM 不只是「内存」，是缓存+管理+保护三合一** — 不要只从「内存」角度理解",
            "**malloc 实现是用户态，但最终靠 syscall 向内核要页** — malloc 不是系统调用，sbrk/mmap 才是",
            "**地址翻译对程序员透明但不等于无代价** — TLB miss 时页表 walk 需要数十周期；HFT 用大页+绑核减少代价",
        ],
        "quiz": [
            ("Q1: 虚拟内存的三个角色分别是什么？",
             "1) 缓存工具：DRAM 缓存磁盘（swap），按需分页；2) 管理工具：隔离进程、简化链接/加载/共享；3) 保护工具：PTE 权限位控制访问。"),
            ("Q2: 地址翻译的完整路径是什么？TLB 在哪里？",
             "VA → TLB 查找（hit 直接得 PA）→ miss 则 walk 多级页表 → 得 PTE → 检查权限/存在位 → page fault 或 PA → L1 cache。TLB 在 MMU 内，缓存 VPN→PPN 映射。"),
            ("Q3: HFT 减少虚拟内存相关延迟的手段有哪些？",
             "1) 大页（2MB/1GB）减少 TLB 项；2) CPU 绑定减少上下文切换（避免刷 TLB）；3) mlock 防止页换出；4) 预 fault 热数据（MAP_POPULATE）；5) 对象池避免 malloc。"),
            ("Q4: malloc、sbrk、mmap 三者的关系？",
             "malloc 是用户态库函数，管理堆空闲链表。堆不够时调 sbrk/brk（调整 program break）或 mmap（大块分配）向内核申请内存。sbrk 和 mmap 是系统调用，malloc 是封装。"),
        ],
    },
]


def build_replacement(entry):
    """构建替换文本：扩充内容（如有）+ 陷阱 + 折叠自测题"""
    parts = []

    # 扩充内容（仅对内容过薄的文件）
    if entry["expand"]:
        parts.append(entry["expand"])

    # 常见陷阱
    parts.append("### 常见陷阱\n")
    for i, trap in enumerate(entry["traps"], 1):
        parts.append(f"{i}. {trap}\n")
    parts.append("\n")

    # 折叠自测题
    parts.append("### 自测题\n\n")
    for q, a in entry["quiz"]:
        parts.append(f"<details>\n<summary>{q}</summary>\n\n{a}\n\n</details>\n\n")

    return "".join(parts)


def process_file(entry):
    filepath = os.path.join(NOTES_DIR, entry["filename"])
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    if SHELL_PATTERN not in content:
        print(f"  [SKIP] {entry['filename']} — 未找到空壳自测模式")
        return False

    replacement = build_replacement(entry)
    new_content = content.replace(SHELL_PATTERN, replacement.rstrip())

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"  [OK] {entry['filename']}")
    return True


def main():
    print(f"Ch9 虚拟内存批量改造 — 共 {len(SECTIONS)} 个文件\n")

    success = 0
    skipped = 0
    for entry in SECTIONS:
        if process_file(entry):
            success += 1
        else:
            skipped += 1

    print(f"\n完成：{success} 成功，{skipped} 跳过")

    # 验证
    import glob
    files = glob.glob(os.path.join(NOTES_DIR, "*.md"))
    shell_remaining = 0
    details_count = 0
    for f in files:
        with open(f, "r", encoding="utf-8") as fh:
            c = fh.read()
            if "（待口述补）" in c:
                shell_remaining += 1
            details_count += c.count("<details>")

    print(f"\n验证：残留空壳 {shell_remaining} 个，<details> 标签 {details_count} 个")


if __name__ == "__main__":
    main()
