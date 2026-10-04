# 7.1 背景知识

> 底本：《BPF之巅》第 7 章 内存，7.1 节（印刷 p256–263）。Linux 虚拟内存机制：每个进程有独立虚拟地址空间，实际使用时才映射物理内存 → 允许超额使用（overcommit）。

## 7.1.1 内存基础知识

### 内存分配器（图 7-1）

```
用户态：进程 → libc 分配器（malloc/free）→ [堆 Heap] → DRAM
内核态：内核 → slab 分配器 → 页分配器 → 空闲列表 → DRAM
```

- **堆**：进程虚拟地址空间中的动态区间；libc malloc/free 管理，free 的内存记录在空闲列表供下次 malloc 复用
- libc 只在内存耗尽时扩展堆（brk），**一般不收缩堆**（虚拟内存不占物理内存，留着无害）
- 替代分配器：**tcmalloc / jemalloc**；JVM 等运行时有自带分配器 + GC，可能把内存放在堆外的私有段

### 内存页生命周期（图 7-2，五步）

1. 应用发起分配（malloc）
2. 分配器空闲列表响应，或先扩展地址空间：**a. `brk()` 扩堆** / **b. `mmap()` 新建段**
3. 应用 store/load 访问 → MMU 查虚拟→物理映射 → **没有映射！→ 缺页错误（page fault）**
4. 内核缺页处理：从物理空闲列表取一页建立映射 → 进程实际占用物理内存（**RSS** 常驻集增长）
5. 内存紧张时 **kswapd 页换出守护进程**扫描回收，可释放三类页：
   - **文件系统页（干净）**：有磁盘备份，直接丢弃可重读
   - **脏页**：先写回磁盘再释放
   - **匿名页**：无文件来源，写入 **swap 换页设备** 后释放（Linux 术语"换页"专指匿名页进出 swap）

**频率分层（图 7-2 粗细箭头，决定 BPF 跟踪开销）**：

| 路径 | 频率 |
|------|------|
| load/store、MMU 查表 | 每秒**数十亿**次（不可跟踪） |
| malloc/free 分配 | 每秒**数百万**次（跟踪开销大） |
| brk/mmap、缺页错误、页换出 | 相对**低频**（跟踪开销可忽略）|

### kswapd 唤醒模式（图 7-3）

- 空闲内存 **低于低阈值** → kswapd 后台唤醒，扫描活跃/非活跃 **LRU 链表**（虽名 scanner，内核早已用链表管理）
- 后台回收不影响应用（除非 CPU/IO 极紧张）
- 空闲内存**跌破最低阈值**且 kswapd 跟不上 → **直接回收（direct reclaim）**：回收在**前台**执行，**内存分配阻塞**直到有页释放 — 性能杀手
- 直接回收还会调内核 shrinker 收缩函数（slab 缓存等）

### 换页设备与 OOM Killer

- swap 允许降级运行（不常用页换出），但速度大跌；**很多生产系统干脆不配 swap**（Netflix 云实例）：与其换页不如负载均衡切到健康实例
- 无 swap + 内存耗尽 → **OOM Killer**：杀掉除内核关键任务和 init(PID 1) 外**占用内存最多**的进程（有全局/每进程调参）
- **页压缩**：内存碎片化后内核 compaction 移动页面凑连续区间
- **文件系统缓存**：空闲内存被借作 page cache（"free 不断变小"是正常现象）；`vm.swappiness` 调节优先丢缓存还是换页

## 7.1.2 BPF 分析能力

BPF 能回答的问题：RSS 为什么涨？缺页来自哪些代码路径/文件？谁阻塞在换入？OOM 时系统状态？哪些路径在分配内存？哪些分配长期不释放（疑似泄漏）？

事件源（表 7-1）：

| 事件 | 事件源 |
|------|--------|
| 用户态内存分配 | uprobes 跟踪分配器函数 / USDT 跟踪 libc |
| 内核态内存分配 | kprobes + kmem 跟踪点 |
| 堆扩展 | brk 系统调用跟踪点 |
| 共享内存函数 | 系统调用跟踪点 |
| 缺页错误 | kprobes、软件事件、exceptions 跟踪点 |
| 页迁移 / 页压缩 / VM 扫描 | migrate / compaction / vmscan 跟踪点 |
| 内存访问周期 | PMC |

libc 自带 **USDT 探针**（`bpftrace -l usdt:...libc...` 可列出 30+ 个）：`memory_heap_new`、`memory_sbrk_more`、`memory_arena_new`、`memory_malloc_retry` 等。

**开销警示**：分配事件每秒数百万次，跟踪可造成 ~10% 损耗，极端情况下**速度降至 1/10** → 优先跟踪低频事件（缺页/换页/brk/mmap）；malloc 调用路径可用 profile 采样粗粒度替代；未来用户态 uprobes（无内核陷）有望提速 10–100 倍。

## 7.1.3 分析策略（九步）

1. `dmesg` 查 OOM Kill 记录
2. 查 swap 配置与活跃 I/O（swapon/iostat/vmstat si-so）
3. 全系统内存与缓存（free）
4. 按进程内存用量（top/ps）
5. 缺页错误频率 + 调用栈（解释 RSS 增长）
6. 缺页关联的文件
7. 跟踪 brk/mmap 换个角度审查
8. 上 BPF 工具（7.3）
9. PMC 测缓存命中/内存访问（PEBS 定位到指令）

## HFT 关联

- 交易机**不配 swap**是标配（延迟敏感不接受降级运行）→ OOM Killer 是唯一出口：策略进程内存必须有硬上限 + oom_score_adj 保护关键进程
- 直接回收 = 毫秒级分配阻塞 — 策略启动预分配全部内存 + mlockall 锁页 + `vm.min_free_kbytes` 调大，让 kswapd 早启动、避免前台回收
- 缺页错误 = 一次 µs 级停顿：热路径数据结构预触碰（prefault）+ 巨页（hfaults 验证）是低延迟标配

## 常见陷阱

1. **看到 free 列小就以为内存不足** — 应看 available（含可回收缓存）
2. **以为 malloc 就占物理内存** — 虚拟地址而已，首次访问缺页才占（RSS 才是真相）
3. **swap 有 I/O 但不影响性能的错误认知** — 换入（swapin）才直接阻塞应用；扫描/换出指标是间接的
4. **忽略直接回收的存在** — vmstat si/so 为 0 不代表没事，无 swap 系统压力全走直接回收（用 vmscan/drsnoop 看）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **从 malloc 到物理内存占用的完整链条？**

   <details>
   <summary>参考答案</summary>

   malloc（libc 空闲列表 or brk 扩堆/mmap 建段，只是虚拟地址）→ 应用首次 store/load → MMU 查映射失败 → 缺页错误 → 内核缺页处理函数从空闲列表取物理页建映射 → RSS 增长。
   </details>

2. **kswapd 后台回收和直接回收的区别？为什么直接回收是性能杀手？**

   <details>
   <summary>参考答案</summary>

   后台回收由 kswapd 在空闲内存低于低阈值时异步执行，不阻塞应用。直接回收发生在空闲内存跌破最低阈值、kswapd 跟不上时：回收逻辑在分配进程的上下文前台执行，内存分配请求阻塞到有页释放 — 应用毫秒级停顿。调优方向是 vm 参数让后台回收提前开始。
   </details>

3. **哪些内存事件适合 BPF 跟踪、哪些不适合？**

   <details>
   <summary>参考答案</summary>

   适合（低频、开销可忽略）：缺页错误、页换出、brk/mmap 调用、OOM、vmscan、页迁移/压缩。不适合（高频）：malloc/free 每秒百万次（跟踪可掉 10% 甚至 10 倍性能）、load/store 和 MMU 查表（每秒数十亿次，纯硬件路径）。malloc 路径分析可用 profile 定时采样粗粒度替代。
   </details>

</details>
