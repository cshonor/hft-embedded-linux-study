# Ch 4 §6 Linux 2.6 内核的新变化 → v6.6（vDSO / 布局 / 现代对照）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`arch/x86/entry/vdso/`、`mm/mmap.c`）

---

## 本节讲什么

原书章末三个 2.6 新特性（vsyscall、4G/4G 分割、非线性映射）接到 v6.6 的现状，顺带把本章六节收拢成"进程地址空间"的完整体检清单。

---

## 1. 三个 2.6 特性的今生前世

### ① vsyscall → vDSO

| | vsyscall（2.6 初） | vDSO（现代） |
|---|---------------------|--------------|
| 形态 | 固定地址 1 页（`0xffffffffff600000`），静态代码 | 动态链接对象 `linux-vdso.so.1`（随机地址） |
| 机制 | 用户态直接执行（内核预映射） | 同，但 **内核按 CPU 特性现场生成代码**（如 TSC 频率、可用指令集） |
| v6.6 状态 | 仅保留极小兼容段（且默认模拟执行防侧信道） | `clock_gettime`/`gettimeofday`/`getcpu` 主通道 |
| HFT | — | **计时热路径的基石**（~20ns 级 vs syscall 100ns+） |

**验证自己在走 vDSO：** `LD_TRACE_LOADED_OBJECTS=1 /bin/true | grep vdso`；strace 里 `clock_gettime` **不出现**（没进内核）即成功。12.5/ch15 延迟测量的 CLOCK_MONOTONIC_RAW 就跑在这上面。

### ② 4GiB/4GiB 分割 → 已完成历史使命

32 位时代让用户/内核各占 4GiB 的补丁（需 TLB 切换开销）——64 位普及后问题消失，**整条支线按历史读**。遗产只剩思想：**内核/用户地址空间分离的极端形态**在现代以 KPTI/TTBR 分离复活（§1 已讲）。

### ③ `MAP_POPULATE` / 非线性映射

| | v6.6 现状 |
|---|-----------|
| `MAP_POPULATE` | 活着且好用：mmap 时同步 prefault（配合 `mlock` 是 HFT 标配） |
| `remap_file_pages` | **2.6→4.x 已弃用**（5.x 语义退化为建 VMA），非线性文件映射被 **稀疏多 VMA** 方案取代——勿学 |

## 2. 本章六节收拢：进程地址空间完整体检

```
进程
  mm_struct（§2）
    ├─ pgd ──► 页表（Ch 3）
    └─ mm_mt ──► VMA[]（§3）
          ├── [exe text/data]      ← PIE 随机
          ├── [heap]               ← brk
          ├── [mmap: 行情 arena]    ← MAP_FIXED_NOREPLACE 定格（§1）
          ├── [mmap: hugetlb 池]
          └── [stack / vdso]

  访问 VA（§4）
    → VMA flags 合法？（第一层）
    → PTE 有效？（第二层，fault 四路分发）
    → 内核↔用户数据搬运走异常表防线（§5）
```

| 体检项 | 命令 | 健康标准（HFT 引擎） |
|--------|------|----------------------|
| 布局稳定 | `maps` 快照 diff | 启动后零变化 |
| VMA 数量 | `wc -l maps` | <100 且恒定 |
| 驻留达标 | `smaps` 的 RSS == 映射预算 | mlock 后无缺量 |
| 零 fault | perf page-faults 计数 | 稳态增量为 0 |
| 无 swap | pswpin/pswpout | = 0 |
| THP 状态 | smaps AnonHugePages | 按策略（never+显式大页应为 0） |
| 计时路径 | strace 看不到 clock_gettime | vDSO 生效 |
| 峰值 | VmHWM | ≤ 预算×1.05 |

## 3. HFT 精读 checklist（终版）

| 目标 | 手段 | 出处 |
|------|------|------|
| 消除运行时 fault | MAP_POPULATE + touch + mlock | §4 |
| 禁止 swap | mlockall + 容量规划 | §4 |
| 共享只读行情 | MAP_SHARED 只读 + 每进程写副本 | §3 |
| 避免 COW 风暴 | 线程模型 / fork 后冻结写 | §4 |
| syscall 延迟 | vDSO 计时 + 批量 copy | §5/本节 |
| 布局确定性 | MAP_FIXED_NOREPLACE + 关 ASLR | §1 |
| mmap_lock 零写事件 | 布局定型 + THP=never + 无运行期 m(un)map | §2 |

## 4. 衔接（向后看）

- [Ch 5 启动内存分配器](../../chapter-05-boot-memory-allocator/)：这一切在 boot 期的起点
- [Ch 8 slab](../../chapter-08-slab-allocator/)：缺页落下的页被内核怎么用
- [Ch 10 回收](../../chapter-10-page-frame-reclamation/)：mlock 之外的页都活在谁的威胁下
- [12.5 全系列](../../../12.5-modern-networking/)：本章机制在网络/IO 路径的兑现
- [06.5/ch05 maple tree](../../../06.5-modern-mm/chapter-05-vm-address-space-maple-tree/)：mm_mt 的专项深读

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 vDSO 代码要内核"现场生成"而不是静态一份？**
A：它做的事依赖 CPU 具体能力：TSC 是否 invariant、rtdscp 有无、时钟源是 TSC 还是 hpet——静态代码只能取交集（慢路径保底）。现场生成 = 每台机器拿到为自己 CPU 定制的最快版本。**"能力探测 + 特化代码"是跨硬件性能软件的标准形态**（DPDK 的 CPU 微架构分支同构）。

**Q2：`clock_gettime` 一定走 vDSO 吗？哪些情况会退回 syscall？**
A：用户显式 `syscall()` 直调、静态链接的极老 glibc、时钟源为非 TSC 的 CLOCK_REALTIME_COARSE 之外某些源（如 hpet 上 CLOCK_MONOTONIC_RAW 部分版本）——具体看内核 `__vdso_clock_gettime` 的时钟源 case。验证永远用 strace 实测，别信文档。

**Q3：4G/4G 分割的"TLB 切换开销"在 64 位还存在吗？**
A：以别的形式存在：KPTI 的双表切换（Ch3）。历史教训值得背：**每次用户/内核地址空间彻底分离，都要付出切换税**——安全与性能在同一处的永恒拔河。

**Q4：`MAP_POPULATE` 和自己写循环 touch 有差吗？**
A：细节差：POPULATE 在 mmap 系统调用内批量 fault（持锁路径更短、批量装 PTE）；touch 循环每页一次独立 fault（可观测、可中途校验内容）。生产用 POPULATE+mlock；需要验证每页真驻留（防零页假象）时用 touch 循环。

**Q5：怎么给引擎做"地址空间回归测试"？**
A：快照 diff 法：启动稳定后 `cat /proc/pid/maps > golden.maps`；每次发版后 diff——任何新增段（新依赖库/意外 mmap）= 回归。配套 `smaps` 快照盯 RSS 分布。这比任何文档都诚实地守住 §3 的"布局定型"纪律。

</details>

---
