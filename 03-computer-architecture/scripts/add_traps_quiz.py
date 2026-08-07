#!/usr/bin/env python3
"""03-computer-architecture Ch2+Ch5 新手化增强脚本。

为 14 篇🔴精读笔记添加「常见陷阱(3) + 折叠自测题(3)」。
笔记结尾为 `---`，在最后一个 `---` 前插入。
"""

import os, re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MOD03 = os.path.dirname(SCRIPT_DIR)  # 03-computer-architecture

CH2 = "chapter-02-memory-hierarchy-design"
CH5 = "chapter-05-thread-level-parallelism"

def make_block(traps, quiz):
    """Generate the 常见陷阱 + 自测题 block."""
    lines = ["", "### 常见陷阱", ""]
    for t in traps:
        lines.append(f"- {t}")
    lines.append("")
    lines.append("### 自测题（点击展开）")
    lines.append("")
    for i, (q, a) in enumerate(quiz, 1):
        lines.append(f"<details>")
        lines.append(f"<summary>Q{i}. {q}</summary>")
        lines.append("")
        lines.append(f"{a}")
        lines.append("")
        lines.append("</details>")
        lines.append("")
    return "\n".join(lines)

def insert_before_final_sep(content, block):
    """Insert block before the last `---` separator."""
    # Find the last --- line
    idx = content.rfind("\n---\n")
    if idx == -1:
        # Try --- at end without trailing newline
        idx = content.rfind("\n---")
        if idx == -1:
            # Just append
            return content + "\n" + block + "\n"
        return content[:idx+1] + block + "\n" + content[idx+1:]
    return content[:idx+1] + block + content[idx+1:]

# ── Ch2 陷阱 + 自测 ──
NOTES_CH2 = {
    f"{CH2}/notes/section-2.1-引言与存储器层次.md": (
        [
            "把 AMAT 公式当唯一指标 — AMAT 只反映平均延迟，HFT 热路径看的是 **尾部延迟**（P99 miss penalty），一次 L3 miss 就可能毁掉整个 tick 预算",
            "认为「L1 命中率 99% 就够了」— 1% 的 L2/L3 miss 贡献了绝大部分时间（AMAT 中 miss rate × penalty 主导），看 miss 次数而非命中率",
            "忽略 DRAM 延迟几乎不降的事实 — DDR 带宽翻倍但随机访问延迟仍在 ~100ns，用顺序带宽推测随机延迟会严重高估性能",
        ],
        [
            ("AMAT = 4 周期（L1 hit）+ 5% × 100 周期（L2 miss penalty）。AMAT 是多少？哪一项主导？",
             "AMAT = 4 + 0.05 × 100 = 9 周期。L1 命中贡献 4，miss 贡献 5 — **miss 主导**，尽管命中率 95%。这说明降低 miss rate 比降低 hit time 更有效。"),
            ("HFT 热路径上，一次 LLC miss ≈ 40-100ns。在 3GHz CPU 上等价于多少周期？对 tick 预算意味着什么？",
             "40-100ns ≈ 120-300 周期。如果 tick 预算是 1μs（3000 周期），一次 LLC miss 就吃掉 4-10% 预算。多次 miss 可直接导致超时。"),
            ("存储墙（Memory Wall）是什么？为什么 DRAM 带宽增长远快于延迟下降？",
             "存储墙指 CPU 速度增长远快于 DRAM 延迟改善。带宽通过多 Bank 交错、更宽接口（DDR→DDR5）提升，但 **随机访问延迟受物理限制**（行激活、列访问的 RC 延迟）几乎不降。→ 局部性愈发重要。"),
        ],
    ),
    f"{CH2}/notes/section-2.2-存储器技术与优化.md": (
        [
            "用 DRAM 带宽规格推算随机访问性能 — DDR5 标称 64GB/s 是 **顺序突发** 带宽；随机指针追逐的实际带宽可能只有 1/10",
            "认为 HBM 会取代 DDR 做交易主机主存 — HBM 容量 per stack 有限（~16-32GB），成本高，交易主机仍以 DDR + L3 为主；HBM 主要用于 FPGA/加速卡",
            "忽略 Flash 写放大对日志系统的影响 — SSD 写放大系数可达 2-4x，大量小写（如逐 tick 日志）会加速磨损并引入延迟尖刺",
        ],
        [
            ("SRAM 和 DRAM 的根本区别是什么？为什么 L1 用 SRAM 而不用 DRAM？",
             "SRAM 不需刷新（静态），速度快但密度低（每 bit 6 晶体管）；DRAM 需周期刷新（动态），密度高（每 bit 1 电容+1 晶体管）但慢。L1 要求 1-4 周期访问，只有 SRAM 能满足；DRAM 的刷新周期和 RC 延迟使其无法做 L1。"),
            ("多 Bank DDR 如何提高有效带宽？为什么对 HFT 行情 buffer 有意义？",
             "多 Bank 允许 **交错访问** — 一个 Bank 在行激活/预充电时，另一个 Bank 可以传输数据。行情 buffer 顺序读 → 多 Bank 并行 → 接近峰值带宽。随机访问则无法利用 Bank 交错。"),
            ("HFT 日志系统用 NVMe SSD。写放大是什么？为什么 fsync 在热路径上要避免？",
             "写放大 = 实际写入 NAND 的数据量 / 主机写入的数据量。SSD 以页写、块擦，小写被放大。fsync 强制刷盘 → 触发写缓冲 flush → 延迟尖刺。热路径应 **异步持久化**，不阻塞 tick 处理。"),
        ],
    ),
    f"{CH2}/notes/section-2.3-缓存性能十项高级优化.md": (
        [
            "认为硬件预取总是有益 — 对 **指针追逐、哈希表** 等不可预测访问，预取拉入无用 cache line → **预取污染**，挤掉热数据反而变慢",
            "对 false sharing 只做 alignas(64) 就放心了 — 还需确保 **不同核不写同一 padded 结构的不同字段**；per-thread 私有计数器 + 周期合并才是正解",
            "分块（Tiling）大小不匹配 cache 容量 — 分块太大 → 工作集溢出 L1/L2 → miss 暴涨；必须用 `perf stat` 实测确定分块大小",
        ],
        [
            ("十项优化中，哪些是 **软件可控** 的？哪些是纯硬件的？",
             "软件可控：#7 编译器分块/循环交换、#9 编译器预取。其余 8 项（小 L1、路预测、多 Bank、非阻塞、关键字优先、写合并、硬件预取、HBM）由 CPU 微架构决定，程序员只能间接利用（如数据布局适配硬件预取）。"),
            ("什么是 false sharing？给出一个 HFT 场景的具体例子和对策。",
             "两核各写同一 cache line 内不同变量 → MESI 反复 invalidate → line 在核间乒乓。例：`struct { atomic<u64> order_count; atomic<u64> cancel_count; }` 两核各写一个字段。对策：`alignas(64)` 分 line，或 **per-thread 私有计数器**，周期合并。"),
            ("关键字优先（Critical Word First）为什么对乱序 CPU 重要？没有它会怎样？",
             "cache miss 时，CPU 急需的 **关键字** 先返回，不等整 line 填满。没有它，CPU 要等整 line（64B）传输完才能继续 → 依赖该数据的指令全部 stall 更久。乱序 CPU 可以在关键字到达后立即执行依赖指令，其余 line 填充与执行并行。"),
            ("`__builtin_prefetch` 什么时候有用？什么时候有害？",
             "有用：访问模式 **极稳定** 的热循环（如预计算链表下一节点）。有害：访问不可预测（哈希表、指针追逐）→ 预取无用 line → 污染 cache → 挤掉热数据。务必 profile 对比。"),
        ],
    ),
    f"{CH2}/notes/section-2.4-虚拟内存与虚拟机.md": (
        [
            "热路径没 touch 全部页就上线 — 首次访问触发缺页 → **延迟尖刺**。必须在启动时 touch 全部热页 + mlock 锁定",
            "依赖 THP 自动管理大页 — THP 的 **后台碎片整理/合并** 会引入不可预测的延迟尖刺；实盘应显式 hugepage 或关闭 THP",
            "以为 VM 环境的延迟差异可忽略 — 嵌套虚拟化/设备虚拟化引入 **不可控抖动**；P99 延迟可能比裸金属高 2-5x",
        ],
        [
            ("TLB miss 的代价是什么？为什么 hugepage 能减少 TLB miss？",
             "TLB miss → 遍历多级页表（x86-64 四级 → 4 次内存访问）→ ~100ns。4KB 页：1GB 需 262144 个 TLB 项；2MB hugepage 只需 512 项 → TLB 覆盖范围扩大 512x → miss 大幅减少。"),
            ("HFT 热路径上线前要做哪三件事来避免缺页？",
             "1) `mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定内存 → 防止 swap\n2) 启动时 **touch 全部热页** → 触发缺页提前完成\n3) 使用 **显式 hugepage**（`mmap(MAP_HUGETLB)`）→ 减少 TLB miss"),
            ("SR-IOV 或设备直通对 HFT 网络有什么意义？",
             "SR-IOV/直通让网卡 **绕过 hypervisor** → 减少虚拟化层开销和上下文切换 → 降低网络延迟抖动。虚拟化网络路径（vSwitch）会增加不可预测延迟，不适合延迟敏感生产。"),
        ],
    ),
    f"{CH2}/notes/section-2.5-交叉领域问题.md": (
        [
            "忽略 DMA 与 CPU cache 的一致性问题 — 网卡 DMA 写入内存后，CPU 可能仍读 **旧 cache 副本** → 数据错误；需 barrier 或 cache-coherent DMA",
            "认为推测执行总是安全的 — 推测加载可能触发 **本不该执行的页故障/权限错**；Spectre/Meltdown 就是利用推测执行的侧信道",
            "DPDK 轮询模式下忘记内存序 — 即使用户态轮询，也需确保 **读到设备写入** 的正确时序（rmb/wmb 或 volatile + atomic）",
        ],
        [
            ("什么是 cache-coherent DMA？为什么网卡 DMA 需要 it？",
             "DMA 设备直接写内存，CPU cache 可能仍有旧副本。cache-coherent DMA（通过 IOMMU/snoop）确保设备写入后，CPU cache 对应 line 被失效或更新。无一致性 → CPU 读到 **过期数据**。"),
            ("推测执行如何影响内存系统？对 HFT 代码有什么启示？",
             "乱序 CPU 可能在分支解决前推测访问内存 → 可能触发本不该发生的缺页/权限异常。启示：热路径的 **指针访问如果无分支依赖** 更容易被推测/预取帮助；不可预测分支 + 指针追逐 → IPC 下降、预取失效。"),
            ("DPDK 用户态轮询为什么仍需关心 cache 一致性？",
             "DPDK mmap UIO/vfio → 网卡 DMA 写 descriptor ring → CPU 轮询读。即使无内核参与，CPU cache 和设备写入之间 **仍需内存序保证**（rmb 确保看到数据后才读 ring tail）。无 barrier → 可能读到半更新状态。"),
        ],
    ),
    f"{CH2}/notes/section-2.6-实例分析-Cortex-A53与Core-i7.md": (
        [
            "用 Cortex-A53 的 cache 行为推断 i7 性能 — 两者预取策略（保守 vs 激进）、缓存层次（2 级 vs 3 级）、TLB 大小差异极大；行为不可互相推断",
            "以为 i7 激进预取一定更快 — 对 **不规则访问模式**（哈希表、指针追逐），激进预取可能 **预取污染** → 反而比保守预取慢",
            "换同代不同 SKU 不重新压测 — L3 容量差异（如 8MB vs 12MB）可能显著改变 replay 性能，尤其是多策略争用 LLC 的场景",
        ],
        [
            ("Cortex-A53 和 Core i7-6700 在缓存层次和预取策略上的主要差异是什么？",
             "A53：2 级 cache，保守预取（省电、可预测）。i7：3 级 cache（L3 共享），**激进预取**（为顺序扫描/规则 stride 优化）。A53 为功耗设计，i7 为性能设计。"),
            ("为什么 i7 的激进预取对某些 HFT 负载反而有害？",
             "激进预取对 **顺序扫描** 友好，但对 **哈希表/指针追逐** 等不规则访问 → 预取无用 line → 挤掉热数据 → miss rate 上升 → 性能下降。HFT 订单簿查找、链表遍历等场景需 profile 确认预取是否有害。"),
            ("HFT colo 服务器为什么要注意 L3 共享？绑核有什么帮助？",
             "L3 多核共享 → 多策略/多线程工作集之和超 L3 → **互挤失效**（LLC eviction）。绑核隔离 → 每核工作集独享 L3 分区（Intel CAT 可硬件隔离）→ 减少 cross-core eviction → 降低 miss rate。"),
        ],
    ),
    f"{CH2}/notes/section-2.7-谬误与陷阱.md": (
        [
            "用 microbench 的 L1 命中率推断端到端性能 — 端到端含内核态切换、网卡 DMA、队列等待；L1 命中率 99% 不等于端到端快",
            "认为容量越大总是越好 — 大 cache → 命中时间↑、功耗↑；存在甜点，超出后收益递减甚至变差（tag 查找延迟增加）",
            "峰值内存带宽 = 你的带宽 — 随机访问、跨 NUMA、多核争用远低于峰值；实际带宽可能只有标称的 10-30%",
        ],
        [
            ("为什么不能用程序 A 的 cache 行为预测程序 B？",
             "访问模式差异极大 — 顺序扫描 vs 随机查找 vs 指针追逐 → miss rate、预取效果、bank 利用率完全不同。必须 **实测自己的热路径**（`perf stat -e cache-misses,LLC-load-misses`）。"),
            ("THP 导致延迟尖刺的机制是什么？对策有哪些？",
             "THP 后台碎片整理/合并 → khugepaged 线程扫描+合并 4KB 页为 2MB → 可能在热路径执行时触发 **内存整理/迁移** → 延迟尖刺。对策：1) 显式 hugepage（`mmap(MAP_HUGETLB)`）2) `echo never > /sys/kernel/mm/transparent_hugepage/enabled` 关闭 THP 3) 环境相关，需压测确认。"),
            ("HFT 特有的 5 个 cache 陷阱是什么？各举一个对策。",
             "1) false sharing → alignas(64)/per-thread 计数器\n2) 冷热数据混 line → SoA/结构拆分\n3) LLC 被其他进程污染 → isolcpus/cgroup/专用机\n4) THP 延迟尖刺 → 显式 hugepage\n5) microbench ≠ 端到端 → 实测完整路径"),
        ],
    ),
}

# ── Ch5 陷阱 + 自测 ──
NOTES_CH5 = {
    f"{CH5}/notes/section-5.1-引言与多处理器挑战.md": (
        [
            "以为加核就能线性加速 — Amdahl 定律：串行段封顶加速比；订单簿单连接排序等串行段不消除，加核无益",
            "忽略 NUMA 拓扑 — 跨 socket 访问共享写变量 → 目录协议 + 远程 hop → **延迟尖刺**；行情核与发单核必须同 socket",
            "不关闭 NUMA balancing — 内核自动迁移页 → 运行时 **不可预测延迟**；实盘应 `numa_balancing=0` 换确定性",
        ],
        [
            ("Amdahl 定律：程序 90% 可并行，10% 串行。无限核加速比上限是多少？",
             "S = 1 / (0.1 + 0.9/∞) = 1 / 0.1 = **10x**。即使无限核，10% 串行段将加速比封顶在 10x。→ 先 profile 找串行段，再决定并行策略。"),
            ("UMA 和 NUMA 的区别？HFT 为什么要绑核 + NUMA 本地分配？",
             "UMA：所有核对内存均匀延迟（SMP，≤数十核）。NUMA：内存分节点，本地快远程慢（DSM）。HFT 绑核 + 本地分配 → 行情 buffer 和处理线程 **同节点** → 避免跨 socket 访问的 ~2x 延迟惩罚。"),
            ("HFT 服务器上为什么要关闭 `numa_balancing`？怎么关？",
             "内核 NUMA balancing 自动把页迁移到「访问最频繁」的节点 → 运行时迁移 = 不可预测延迟。关闭：`echo 0 > /proc/sys/kernel/numa_balancing` 或启动参数 `numa_balancing=disable`。"),
        ],
    ),
    f"{CH5}/notes/section-5.2-缓存一致性与监听协议.md": (
        [
            "混淆 Coherence 和 Consistency — Coherence 管 **同一地址** 的一致性；Consistency 管 **不同地址** 的访问顺序。两者是不同层次的问题",
            "认为 MESI 的 S 态（共享只读）总是安全 — 多核同时持有 S 态没问题，但任一核写 → 其他核 invalidate → **乒乓**；只读共享 OK，写多则性能暴跌",
            "忽略 MOESI 的 O（Owned）态优化 — O 态允许脏块由所有者直接提供给读请求，避免写回内存；不理解会导致错误分析一致性流量",
        ],
        [
            ("MESI 四个状态分别是什么？M 和 E 的区别是什么？",
             "M=Modified（独占且脏，需写回）、E=Exclusive（独占且干净，无需写回）、S=Shared（多核只读）、I=Invalid。M 和 E 都是独占，但 M 已修改（与内存不一致），E 未修改（与内存一致）。写 E 态 line → 直接转 M，无需总线事务。"),
            ("写失效（Write Invalidate）为什么比写更新（Write Update）更常用？",
             "写失效：写前使其他核副本失效，之后独占写，**无后续总线流量**（直到其他核再读）。写更新：每次写都广播新值 → 总线流量大。写失效的一次性开销通常低于写更新的持续性流量。"),
            ("HFT 场景：策略配置表是只读共享的。MESI 的哪个状态最友好？为什么？",
             "**S 态（Shared）**。多核只读同一 cache line → 都持有 S 态副本 → 无一致性流量。只要不写，S 态就是零开销共享。→ 配置表设计为 **启动后不可变**（const/read-only）。"),
        ],
    ),
    f"{CH5}/notes/section-5.3-性能分析与伪共享.md": (
        [
            "只关注 true sharing 而忽略 false sharing — true sharing 是语义必要的；false sharing 是 **协议误伤**，可通过 padding 消除",
            "用 `alignas(64)` padding 后不做实测 — 某些编译器/ABI 可能不在结构体间插入 padding；需 `perf c2c` 或 vtune 确认 false sharing 已消除",
            "以为只读共享不会触发一致性流量 — 正确，但 **写一个字段就会使整 line 在其他核失效**；混在热结构里的冷写也会污染只读字段",
        ],
        [
            ("什么是 false sharing？它和 true sharing 的区别是什么？",
             "True sharing：多核读写 **同一变量** — 语义需要的一致性流量。False sharing：多核写 **不同变量** 但在同一 cache line → MESI 以 line 为粒度失效 → **不必要的乒乓**。False sharing 是纯性能损失，可消除。"),
            ("给出 false sharing 的代码示例和 3 种对策。",
             "示例：`struct { atomic<u64> c1; atomic<u64> c2; }` 两核各写 c1/c2 → 同 line 乒乓。\n对策：1) `alignas(64)` 分 line 2) **per-thread 私有计数器**，周期合并 3) 热写结构 **按核分片** 或单写者。"),
            ("影响 SMP 性能的 4 个因素是什么？cache 块大小如何影响 false sharing？",
             "1) 缓存容量（工作集 vs 容量失效）2) 处理器数量（核越多一致性流量越大）3) **缓存块大小**（块越大 false sharing「误伤」范围越大）4) 工作负载（锁竞争 vs OS 干扰）。64B line → 8 个 u64 可能同 line → false sharing 风险高。"),
            ("`perf c2c` 是什么？HFT 中怎么用它？",
             "`perf c2c` = cache-to-cache 工具，检测 **false sharing 热点** — 哪些 cache line 在核间乒乓最频繁。用法：`perf c2c record ./my_app` → `perf c2c report`。看 HITM（Hit Modified）计数高的 line → 定位 false sharing 位置。"),
        ],
    ),
    f"{CH5}/notes/section-5.4-目录式一致性与DSM.md": (
        [
            "以为监听协议（Snooping）可以无限扩展 — Snooping 依赖 **广播**，核数增多 → 总线/互连带宽爆炸 → 必须切目录协议",
            "跨 socket 共享写变量不做分片 — 目录协议 + 远程 hop → 延迟尖刺；应 per-socket 工作集隔离",
            "不关 NUMA balancing 就上生产 — 内核自动迁移页到「热点」节点 → 运行时不可预测延迟；实盘必须关闭",
        ],
        [
            ("为什么 Snooping 协议不适合大规模多核？Directory 协议如何解决？",
             "Snooping 依赖 **广播** → 核数增多时总线/互连带宽爆炸（O(N) 广播）。Directory 为每个内存块维护 **哪些节点有副本**（位向量）→ 只需 **点对点消息** 通知相关节点 → O(相关节点数) 而非 O(全体)。"),
            ("双路 Xeon 服务器上，跨 socket 访问共享写变量为什么会延迟尖刺？",
             "跨 socket → **目录协议查找 + 远程 hop** → 经过互连（UPI/QPI）→ 延迟 2-3x 于本地访问。如果多核频繁写同一变量 → 反复跨 socket invalidate → 延迟尖刺。对策：per-socket 工作集、行情核与发单核同 socket。"),
            ("HFT 实盘为什么常关闭 NUMA balancing？怎么关？",
             "内核 NUMA balancing 自动把页迁移到「访问最频繁」的节点 → 运行时迁移 = 不可预测延迟尖刺。实盘要确定性 → 关闭。`echo 0 > /proc/sys/kernel/numa_balancing` 或启动参数 `numa_balancing=disable`。"),
        ],
    ),
    f"{CH5}/notes/section-5.5-同步基础.md": (
        [
            "在热路径用 mutex 保护长临界区 — mutex 争用 → 进内核 → **微秒级延迟**；热路径应用极短自旋 + atomic 或无锁结构",
            "自旋锁不做 test-test-and-set — 直接原子写 → 每次写触发全总线 invalidate → **总线风暴**；应先读本地副本（test）再 set",
            "以为 LL/SC 不会失败 — SC 可能因其他核写入同 cache line 而 **失败** → 需重试循环；且同 line 的无关写也会导致 SC 失败（false sharing on LL/SC）",
        ],
        [
            ("自旋锁的「缓存一致性友好实现」是什么？为什么比直接原子写好？",
             "**test-test-and-set**：先在 **本地缓存行** 读锁状态（test），只在看到锁释放时才原子写（set）。→ 等待期间不发总线写 → 不产生一致性流量。直接原子写 → 每次写都触发 invalidate → 总线风暴。"),
            ("LL/SC（Load-Linked/Store-Conditional）是什么？SC 失败时怎么办？",
             "LL 读一个地址并标记；SC 只在 **LL 后该地址未被其他核修改** 时才写入成功。SC 失败 → 重试 LL/SC 循环。x86 没有 LL/SC，用 `LOCK CMPXCHG`（CAS）实现类似语义。"),
            ("HFT 热路径什么时候用自旋锁，什么时候用无锁队列？",
             "自旋锁：极短临界区（几条指令），配合 `pause` 指令。**不进内核** → 延迟低。无锁队列（SPSC/MPSC）：生产者-消费者场景，用 CAS/atomic 避免 **锁的争用和缓存乒乓**。长临界区用 mutex 但 **不进热路径**。"),
            ("x86 的 `pause` 指令在自旋锁中起什么作用？",
             "`pause`（REP NOP）：1) 提示 CPU 这是自旋等待 → 减少流水线功耗 2) 避免 **memory ordering violation** 在退出循环时的惩罚 3) 给超线程兄弟线程更多执行资源。自旋循环中 `while (!try_lock()) _mm_pause();`。"),
        ],
    ),
    f"{CH5}/notes/section-5.6-内存一致性模型.md": (
        [
            "在宽松模型上用 `memory_order_relaxed` 读标志 — relaxed 不保证看到 **写数据 → 写标志** 的顺序 → 可能读到 **半初始化对象** → 极难复现 bug",
            "x86 代码直接移植到 ARM 不调整内存序 — x86 是 TSO（较强），ARM 是 **弱模型**；x86 上「碰巧正确」的代码在 ARM 上可能 data race",
            "混淆 Coherence 和 Consistency — Coherence 管单地址值一致；Consistency 管多地址 **观察顺序**；一致性协议不规定跨地址顺序",
        ],
        [
            ("Coherence 和 Consistency 的区别是什么？各回答什么问题？",
             "Coherence：对 **同一地址**，各核看到的值是否一致？（单地址值一致）Consistency：对 **不同地址**，读写以何种 **顺序** 被其他核观察到？（跨地址序）Coherence 不规定 A 写完后何时能看到 B 的写。"),
            ("TSO（Total Store Order）是什么？x86 为什么对程序员较「友好」？",
             "TSO：写可缓冲（store buffer），但 **读不能越过未决写**（同地址读看到最新写）。x86 近似 TSO → 程序员不需要太多显式 barrier → 更容易写正确并发代码。ARM 更弱 → 需要 `dmb`/`acquire-release`。"),
            ("SPSC ring buffer 的 publish 顺序为什么重要？用 C++ atomic 怎么写？",
             "写数据 → release store 索引 → 消费者 acquire load 索引 → 看到索引后才读数据。若用 `relaxed` → 可能 **先更新索引再写数据** → 消费者看到索引但读到旧数据。正确写法：`data[idx] = val; head.store(idx+1, memory_order_release);` / `h = head.load(memory_order_acquire); if (h > tail) { val = data[tail]; }`"),
            ("为什么 x86 代码直接移植到 ARM 可能出 bug？",
             "x86 TSO 较强 → 很多「本该用 barrier」的代码 **碰巧正确**。ARM 弱模型 → 没有 barrier 时重排更激进 → data race 暴露。移植时必须审查所有 **跨线程共享访问** 的内存序，用 `acquire/release` 或 `seq_cst` 显式标注。"),
        ],
    ),
    f"{CH5}/notes/section-5.7-5.11-交叉问题实例与展望.md": (
        [
            "以为 SMT（超线程）对单线程延迟有帮助 — SMT 提升的是 **吞吐**，不是单线程延迟；HFT 关键路径应 **关 HT、独占物理核**",
            "选 CPU 只看核数 — 还需看 **L3 容量、内存通道数、NUMA 拓扑**；L3 太小 → 多策略互挤；内存通道少 → 带宽瓶颈",
            "以为加核不改软件就行 — 伪共享、锁竞争、NUMA 访存模式若不调，加核后性能可能 **倒退**",
        ],
        [
            ("Inclusive 和 Non-inclusive cache 层次各有什么优劣？HFT 关注什么？",
             "Inclusive：L3 包含 L1/L2 副本 → 简化一致性/目录查找，但 **L3 容量被 L1/L2 副本占用**。Non-inclusive：L3 不含 L1/L2 → 容量更大，但一致性维护更复杂。HFT 关注 L3 争用 → inclusive 时多线程工作集之和超 L3 → 互挤失效。"),
            ("为什么 HFT 关键路径要关超线程（HT/SMT）？怎么关？",
             "SMT 两个线程共享物理核的执行资源（ALU/LSU/cache）→ 兄弟线程 **争用资源** → 单线程延迟不稳定。关 HT：BIOS 设置或 `echo 0 > /sys/devices/system/cpu/cpuN/online`（关偶数核的对线程）。关键路径 **独占物理核**。"),
            ("「加核不改软件，性能倒退」的 3 个原因是什么？",
             "1) **伪共享** — 多核写同 line → 乒乓 → 比单核更慢 2) **锁竞争** — 串行段不变，更多核等锁 → Amdahl 限制 3) **NUMA** — 跨 socket 访存 → 延迟增大。→ 加核前先调数据布局、锁粒度、NUMA 绑定。"),
            ("多核扩展的 3 个硬限制是什么？",
             "1) **功耗墙/暗硅** — 不能所有晶体管同时全速 → 加核 ≠ 线性加速 2) **Amdahl** — 串行段封顶 3) **一致性/通信开销** — 核越多一致性流量越大 → 扩展性递减。方向：DSA、专用加速器。"),
        ],
    ),
}

def main():
    all_notes = {**NOTES_CH2, **NOTES_CH5}
    ok, miss = 0, 0

    for rel_path, (traps, quiz) in all_notes.items():
        fpath = os.path.join(MOD03, rel_path)
        if not os.path.exists(fpath):
            print(f"MISSING: {rel_path}")
            miss += 1
            continue

        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        # Skip if already has 常见陷阱
        if "### 常见陷阱" in content:
            print(f"SKIP (already enhanced): {rel_path}")
            ok += 1
            continue

        block = make_block(traps, quiz)
        new_content = insert_before_final_sep(content, block)

        with open(fpath, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"OK: {rel_path}")
        ok += 1

    print(f"\nTotal: {ok} OK, {miss} MISSING")

if __name__ == "__main__":
    main()
