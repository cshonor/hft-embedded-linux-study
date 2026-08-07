#!/usr/bin/env python3
"""Batch add 'common pitfalls' + 'folded quiz' to Ch6 sections."""
import os, re

NOTES_DIR = os.path.join(os.path.dirname(__file__), "..", "chapter-06-memory-hierarchy", "notes")

# Each entry: (filename, traps_text, quiz_text)
SECTIONS = [
("section-6.1-存储技术.md",
"""### 常见陷阱

1. **以为 DRAM 和 cache 差不多快** — DRAM ~50-100ns，L1 ~1ns，差 50-100 倍。一次 DRAM miss 能让 HFT 热路径延迟暴涨上百纳秒。
2. **热路径数据不在 DRAM 就放心了** — DRAM miss 到 DRAM 仍有 ~100ns；HFT 热数据要驻留 L1/L2/L3，不只是「在内存里」。
3. **swap 没禁用** — 热路径数据被换出到磁盘，一次 page fault 就是毫秒级。HFT 服务器必须 `swapoff` + `mlock` 关键内存。""",
"""### 自测题

<details>
<summary>1. SRAM 和 DRAM 的主要区别是什么？各用在哪里？</summary>

**SRAM**：快（~1ns）、贵、低功耗/bit，用于 **cache**（L1/L2/L3）。**DRAM**：慢（~50-100ns）、便宜、需定期刷新，用于 **主存**。HFT 热数据要尽量留在 SRAM（cache）层。
</details>

<details>
<summary>2. memory wall 是什么？为什么 cache 层次越来越深？</summary>

**CPU 速度增长远快于 DRAM 速度增长**，差距（memory wall）持续扩大。为了弥合差距，CPU 增加更多 cache 层级（L1→L2→L3），让热数据留在离核心更近的 SRAM 中。层次结构不会消失。
</details>

<details>
<summary>3. HFT 服务器为什么必须禁用 swap？</summary>

swap 会把内存页换出到磁盘。一旦热路径数据被换出，访问触发 **page fault**，延迟从纳秒暴涨到**毫秒**（慢 10⁶ 倍）。HFT 服务器必须 `swapoff` + `mlock` 锁定关键内存 + 足够 DRAM 装 working set。
</details>"""),

("section-6.2-局部性.md",
"""### 常见陷阱

1. **只关注数据局部性忽略指令局部性** — 大函数/跳转分散的代码 I-cache miss 高。过度展开（§5.8）可能导致代码膨胀超出 I-cache 容量，反而变慢。
2. **链表 vs 数组不区分场景** — 链表节点分散在堆上，cache line 不连续，每次跳转可能 miss。HFT 热路径用连续数组/vector 替代链表，或用节点池预分配保证连续。
3. **以为小对象就一定 cache 友好** — 如果小对象分散 malloc（堆碎片），物理地址不连续，仍然 cache 不友好。关键是**逻辑连续 + 物理连续**。""",
"""### 自测题

<details>
<summary>1. 时间局部性和空间局部性分别是什么？各举一个 HFT 例子。</summary>

- **时间局部性**：刚访问的数据很快再访问。例：热价位节点在多个 tick 中反复更新。
- **空间局部性**：相邻地址即将被访问。例：顺序扫描 `price[]` 数组，cache line 一次拉 64B 覆盖 8 个 double。
</details>

<details>
<summary>2. 步长（stride）如何影响空间局部性？</summary>

步长越大，每次访问跳过的字节越多，空间局部性越差。按行扫二维数组（stride=1）每 64B 拉 1 次 cache line，效率高；按列扫（stride=N×8B）每次访问可能落在不同 cache line，每元素都 miss。
</details>

<details>
<summary>3. HFT 中链表为什么 cache 不友好？怎么改？</summary>

链表节点分散在堆上（malloc 分配），物理地址不连续。遍历时每次 `next` 跳转可能落到不同 cache line → **cache miss**。改用连续数组/vector，或用**节点池**预分配一块连续内存，让节点物理相邻。
</details>"""),

("section-6.3-层次结构与缓存概念.md",
"""### 常见陷阱

1. **混淆 hit rate 和 hit time** — hit rate 高不代表快；如果 hit time 本身高（如 L3 比 L1 慢 10 倍），高 hit rate 仍然慢。AMAT = HitTime + MissRate × MissPenalty，三者都要看。
2. **以为 cache 容量越大越好** — 大 cache 容量意味着更多组/路，查找延迟更高（L3 比 L1 慢就是因为大且远）。层次结构是速度-容量的折中，不是简单「越大越好」。
3. **忽略 cache line 大小** — 即使只读 1 字节，CPU 也会拉整条 cache line（64B）。如果访问模式跨 line（如 misaligned），一次访问触发多次 cache 行填充。""",
"""### 自测题

<details>
<summary>1. 存储器层次结构的核心思想是什么？</summary>

第 k+1 层是第 k 层的 **cache**。上层快但小（寄存器→L1→L2→L3），下层慢但大（DRAM→磁盘）。数据按**块（cache line）**搬移，利用空间局部性。目标是让常用数据留在上层，不常用的留在下层。
</details>

<details>
<summary>2. AMAT 公式是什么？各部分含义？</summary>

**AMAT = HitTime + MissRate × MissPenalty**。HitTime = 命中时的访问时间；MissRate = 缺失概率；MissPenalty = 缺失时向下层取数据的惩罚。HFT 优化方向：降 miss rate（cache 友好布局）或降 miss penalty（预取、NUMA 本地内存）。
</details>

<details>
<summary>3. 为什么 L1 比 L3 快？不只是「更近」。</summary>

L1 容量小（32KB）→ 组数/路数少 → 查找延迟低（tag 比较快）；L3 容量大（数十 MB）→ 查找延迟高。此外 L1 在核心内部（物理距离近），L3 常多核共享（总线更长）。**容量和延迟是折中**——大 cache 查找慢。
</details>"""),

("section-6.4.1-通用组织结构.md",
"""### 常见陷阱

1. **混淆地址划分的位域顺序** — 从低位到高位是 **block offset (b) → set index (s) → tag (t)**。offset 在最低位因为它是块内偏移；index 在中间用于选组；tag 在最高位用于区分同组的不同块。
2. **以为 cache 查找是「先搜所有行」** — 实际是**三步**：①index 选组（直接寻址，不搜索）→ ②tag 比较（只在组内 E 路比较）→ ③valid 位检查。不是暴力搜索所有行。
3. **忘了 valid 位** — 即使 tag 匹配，如果 valid 位为 0，仍然 miss。开机时所有 cache line 的 valid 位都是 0。""",
"""### 自测题

<details>
<summary>1. 物理地址如何划分为 cache 查找所需的三个字段？</summary>

从低位到高位：**block offset (b 位)** — 块内偏移，寻址 64B 内的字节；**set index (s 位)** — 选哪一组；**tag (t 位)** — 区分同组内的不同块。t = 地址总位数 - s - b。
</details>

<details>
<summary>2. cache 查找的三步是什么？</summary>

①**index 选组**：用 set index 直接定位到某一组（不是搜索）；②**tag 比较**：在该组的 E 条 line 中比较 tag 字段；③**valid 检查**：tag 匹配且 valid=1 才命中。三步都通过才算 hit。
</details>

<details>
<summary>3. S、E、B 三个参数分别决定什么？容量怎么算？</summary>

**S = 2^s**（组数）、**E**（每组的 cache line 数，即相联度）、**B = 2^b**（每条 line 的字节数）。容量 ≈ S × E × B。例如 32KB L1：S=64, E=8, B=64 → 64×8×64 = 32768 = 32KB。
</details>"""),

("section-6.4.2-直接映射.md",
"""### 常见陷阱

1. **交替访问映射到同一组的两个地址** — 直接映射 E=1，每组只有 1 条 line。如果地址 A 和 B 的 set index 相同，交替访问会导致**反复踢出**（thrashing），每次都 miss。
2. **以为直接映射没人用** — 实际上 L1 常用直接映射或低路数组相联（速度优先）。直接映射查找最快（只需 1 次 tag 比较），但冲突 miss 高。
3. **混淆 conflict miss 和 capacity miss** — conflict miss 是因为相联度不够（即使总容量够，同组装不下）；capacity miss 是因为总容量不够。直接映射的 conflict miss 最多。""",
"""### 自测题

<details>
<summary>1. 直接映射（E=1）的优缺点是什么？</summary>

**优点**：实现简单、查找最快（只需 1 次 tag 比较）、功耗低。**缺点**：冲突 miss（conflict miss）高——多个不同块映射到同一组时互相踢出（thrashing），即使总容量够也 miss。
</details>

<details>
<summary>2. 什么情况会导致直接映射的 thrashing？</summary>

交替访问两个 set index 相同的地址。例如 `data[0]` 和 `data[64]`（如果数组步长恰好让它们映射到同一组），交替访问时每次都把对方踢出，**每次都 miss**，CPE 暴涨。
</details>

<details>
<summary>3. conflict miss 和 capacity miss 有什么区别？</summary>

**Conflict miss**：总容量够，但相联度不够——同组的 line 装不下多个不同块。**Capacity miss**：总容量不够——工作集超过 cache 容量。直接映射（E=1）的 conflict miss 最多；全相联（E=所有）消除了 conflict miss。
</details>"""),

("section-6.4.3-组相联.md",
"""### 常见陷阱

1. **以为路数越多越好** — E 增加能降低 conflict miss，但增加 tag 比较延迟（E 路 parallel 比较）和硬件面积。L1 通常 8-way，L3 可能 16-way，不是无限增加。
2. **混淆「组相联」和「全相联」** — 组相联有多个组（S>1），每组 E 路；全相联只有 1 组（S=1），所有 line 都在一组。TLB 常用全相联，L1/L2 用组相联。
3. **LRU 替换不是完美策略** — 组相联常用 LRU（最近最少使用）替换，但 LRU 对某些访问模式（如循环扫超过 E 个同组地址）效果差。真 CPU 可能用伪 LRU 省硬件。""",
"""### 自测题

<details>
<summary>1. 组相联为什么是工业界 L1/L2 的主流选择？</summary>

直接映射（E=1）冲突 miss 太多，全相联（E=全部）硬件太贵/太慢。组相联折中：每组 E 路（如 8-way）parallel 比较 tag，降低冲突 miss 同时控制延迟和面积。L1 常用 8-way，查找延迟仍可接受。
</details>

<details>
<summary>2. E 路组相联的 tag 比较是串行还是并行？</summary>

**并行**。E 条 line 的 tag 同时比较（E 个比较器并行工作），命中则输出对应数据。如果串行比较，E 路 8 需要 8 次比较，延迟不可接受。并行比较是组相联的硬件代价——需要 E 个比较器。
</details>

<details>
<summary>3. 组相联如何减少 conflict miss？举例。</summary>

直接映射（E=1）：交替访问映射到同组的 A、B → 每次 miss（thrashing）。组相联（E=8）：A 和 B 都在同一组的 8 条 line 中，可以共存 → 交替访问都命中。只有同组的 9 个不同地址交替访问才会开始踢出。
</details>"""),

("section-6.4.4-全相联.md",
"""### 常见陷阱

1. **以为 L1 应该用全相联** — 全相联查找需要比较所有 line 的 tag（可能上千路），延迟和功耗太高。L1 需要极低延迟，不能用全相联。全相联适合容量小、对冲突 miss 零容忍的场景。
2. **混淆 TLB 和 data cache 的相联度** — TLB 常用全相联或高路组相联（页表项少，值得全相联）；data cache L1/L2 用组相联（容量大，全相联不现实）。
""",
"""### 自测题

<details>
<summary>1. 全相联（S=1）的优缺点是什么？</summary>

**优点**：消除了所有 conflict miss——任何块都可以放在任意 line，只有容量不够时才会 miss。**缺点**：查找时要比较所有 line 的 tag（可能上千路），延迟和功耗太高，不适合大容量 cache。
</details>

<details>
<summary>2. 全相联适合用在哪里？为什么？</summary>

适合**容量小、对 miss 零容忍**的场景：TLB（页表缓存，通常 64-1024 项）、小型查找表。因为项数少，全相联的并行 tag 比较延迟可控；且消除 conflict miss 对 TLB 很重要（TLB miss 代价极高）。
</details>

<details>
<summary>3. 为什么 data cache 不用全相联？</summary>

L1 data cache 32KB = 512 条 64B line，全相联需要 512 个并行 tag 比较器，延迟和面积不可接受。组相联（8-way）只需 8 个比较器，延迟低得多，且 8-way 已经消除了大部分 conflict miss。**速度优先于消除全部 conflict miss**。
</details>"""),

("section-6.4.5-有关写的问题.md",
"""### 常见陷阱

1. **混淆 write-through 和 write-back** — write-through 每次写都同步到下层（简单但总线忙）；write-back 只写 cache，标记 dirty 位，替换时才写回（常用但一致性复杂）。现代 L1/L2 用 write-back。
2. **store miss 时不知道会发生什么** — write-allocate 策略下，store miss 会先 load 整条 cache line 再写入（利用空间局部性）；non-write-allocate 直接写下层。L1 常用 write-allocate。
3. **忽略 store 引发的隐式 load** — write-allocate 下，即使只写 1 字节，也要先从下层拉 64B 的 cache line。如果下层 miss，这个隐式 load 的 miss penalty 和普通 load miss 一样高。""",
"""### 自测题

<details>
<summary>1. write-through 和 write-back 的区别？现代 cache 用哪个？</summary>

**Write-through**：写 cache 同时写下层——简单、一致性好，但每次写都占用总线，功耗高。**Write-back**：只写 cache，标记 dirty 位，替换时才写回——总线占用少、功耗低，但一致性管理复杂。现代 L1/L2 **常用 write-back**。
</details>

<details>
<summary>2. write-allocate 和 non-write-allocate 的区别？store miss 时各发生什么？</summary>

**Write-allocate**：store miss 时先从下层 load 整条 cache line，再写入——利用空间局部性（附近数据可能很快被读）。**Non-write-allocate**：store miss 时直接写到下层，不拉 cache line。L1 常用 write-allocate + write-back。
</details>

<details>
<summary>3. 为什么 store miss 也可能产生 cache miss 延迟？</summary>

Write-allocate 策略下，store miss 会触发**隐式 load**——从下层拉 64B cache line。如果下层也 miss（如 LLC miss 到 DRAM），这个 load 的 miss penalty 和普通 load miss 一样高（~100ns）。HFT 热路径应避免对不在 cache 中的地址做 store。
</details>"""),

("section-6.4.6-真实Cache层次解剖.md",
"""### 常见陷阱

1. **记混 L1/L2/L3 的容量和延迟** — L1 ~32KB/~4 cycles，L2 ~256KB-1MB/~10-15 cycles，L3 ~数 MB/~40 cycles（共享）。记住数量级即可，具体值看 CPU 型号。
2. **忽略 inclusive/exclusive LLC 的影响** — inclusive LLC 意味着 L3 包含 L1/L2 的副本（MESI 一致性简化）；exclusive 意味着 L3 不包含 L1/L2 的数据（容量利用率高但一致性复杂）。影响多核性能。
3. **不知道硬件预取器的存在** — 现代 CPU 有 stride prefetcher，自动检测顺序访问模式并预取。但随机访问模式不会被预取。HFT 可用软件预取补充硬件预取的不足。""",
"""### 自测题

<details>
<summary>1. 典型 x86 服务器 CPU 的 L1/L2/L3 参数是什么？</summary>

L1i/L1d：32KB，8-way，64B line，~4 cycles（私有）；L2：256KB-1MB，~10-15 cycles（私有）；L3/LLC：数 MB-数十 MB，~40 cycles（多核共享）。具体值因 CPU 型号而异，用 `lscpu` 或 `cat /sys/devices/system/cpu/cpu0/cache/` 查看。
</details>

<details>
<summary>2. inclusive 和 exclusive LLC 有什么区别？</summary>

**Inclusive**：L3 包含 L1/L2 中所有数据的副本——一致性协议简单（L3 可作为 snoop filter），但浪费 L3 容量。**Exclusive**：L3 不包含 L1/L2 的数据——L3 容量利用率高，但一致性管理复杂。Intel 常用 inclusive，AMD 常用 exclusive。
</details>

<details>
<summary>3. 硬件预取器如何工作？HFT 如何利用？</summary>

硬件 stride prefetcher 自动检测**顺序访问模式**（如每次 stride=64B），提前预取下几条 cache line。HFT 中：①顺序扫描数组时硬件预取通常有效；②随机访问模式不会被预取——需要软件 `__builtin_prefetch` 补充；③预取太多可能污染 cache（踢出有用数据）。
</details>"""),

("section-6.4.7-Cache参数的性能影响.md",
"""### 常见陷阱

1. **以为增大 cache 容量就能解决所有 miss** — 容量增大降 capacity miss，但不降 conflict miss（如果相联度不变）和 compulsory miss（第一次访问必 miss）。不同 miss 类型需要不同对策。
2. **忽略 cache line 大小对性能的双刃剑效应** — 大 line（128B）提高空间局部性（一次拉更多数据），但增加 miss penalty（拉更多字节）和 false sharing 概率。64B 是当前主流折中。
3. **混淆三种 miss 类型** — Compulsory（冷启动必 miss）、Capacity（工作集 > cache 容量）、Conflict（相联度不够）。HFT 优化重点在 capacity 和 conflict miss。
""",
"""### 自测题

<details>
<summary>1. 三种 cache miss 类型分别是什么？各如何缓解？</summary>

- **Compulsory（冷缺失）**：第一次访问必 miss → 预取（prefetch）
- **Capacity（容量缺失）**：工作集 > cache 容量 → 分块（blocking/tiling）使子块 fit cache
- **Conflict（冲突缺失）**：相联度不够，同组 thrashing → 增大相联度 / 调整数据布局避免同组映射
</details>

<details>
<summary>2. 增大 cache line 大小的利弊是什么？</summary>

**利**：提高空间局部性——一次拉 128B 比 64B 多覆盖一倍数据，顺序访问 miss 减半。**弊**：①miss penalty 增加（拉更多字节）；②false sharing 概率增加（更多线程数据可能落在同一 line）；③浪费带宽（如果只用其中一小部分）。64B 是当前主流折中。
</details>

<details>
<summary>3. HFT 中哪种 miss 最致命？为什么？</summary>

**Capacity miss 和 conflict miss**。Compulsory miss 只在第一次访问发生，后续有 cache；但 capacity/conflict miss 在热循环中**反复发生**，每次 miss penalty ~100ns（到 DRAM），直接导致延迟毛刺。HFT 优化重点：cache 友好的数据布局（降 conflict）+ 控制工作集大小（降 capacity）。
</details>"""),

("section-6.5-编写高速缓存友好的代码.md",
"""### 常见陷阱

1. **按列扫二维数组** — `a[i][j]` 在 C 中是行主序，按列扫（外层 j 内层 i）stride=N×8B，每次跨 cache line → 几乎每元素都 miss。改成按行扫（外层 i 内层 j）stride=8B，64B line 覆盖 8 个元素。
2. **热循环里每包 malloc 小对象** — malloc 分配的堆地址不连续，cache line 分散，每次访问可能 miss。改用预分配的连续数组/对象池。
3. **SoA 和 AoS 选错** — 批量处理某字段时 SoA（该字段连续存储）更好；单条记录多字段同时访问时 AoS 更好。选错会导致不必要的 cache miss。""",
"""### 自测题

<details>
<summary>1. 编写 cache 友好代码的三大原则是什么？</summary>

①**重复引用相同数据**——利用时间局部性（内层循环复用变量）；②**步长为 1 的顺序访问**——利用空间局部性（cache line 一次拉 64B 覆盖多个元素）；③**控制工作集大小**——让它 fit in cache，否则 capacity miss。
</details>

<details>
<summary>2. 为什么按行扫比按列扫快？具体差多少？</summary>

C 中二维数组是行主序，`a[i][j]` 和 `a[i][j+1]` 地址相邻。按行扫 stride=8B，64B cache line 覆盖 8 个 double → 每 8 元素 1 次 miss。按列扫 stride=N×8B，每元素可能跨 line → 几乎每元素都 miss。大数组时差 **10-100 倍**。
</details>

<details>
<summary>3. HFT 中 SoA 和 AoS 各适合什么场景？</summary>

**SoA**（Structure of Arrays）：每个字段单独连续存储。适合**批量处理某字段**（如遍历所有 price 做 sum）——该字段连续，cache 友好。**AoS**（Array of Structures）：每条记录的多个字段连续。适合**单条记录多字段同时访问**——一次拉 64B 覆盖整条记录。按访问模式选。
</details>

<details>
<summary>4. HFT 中为什么热循环禁止每包 malloc？</summary>

malloc 分配的堆地址**不连续**（受堆碎片影响），cache line 分散在 DRAM 各处，每次访问可能 miss。改用**预分配的连续数组/对象池**，让数据物理连续，cache line 相邻，顺序访问命中率高。
</details>"""),

("section-6.6-存储器山.md",
"""### 常见陷阱

1. **以为 stride 越小越好** — stride=1 确实空间局部性最好，但如果 working set 超过 cache 容量，stride=1 仍然 capacity miss。存储器山展示的是 stride **和** working set 的二维关系，不是只看 stride。
2. **分块（blocking/tiling）忘记处理边界** — 分块后每个子块要 fit L1/L2，但矩阵尺寸不一定是块大小的整数倍，需要处理尾部剩余行列。
3. **预取距离算错** — 预取太远（数据还没到使用时间，被挤出 cache）或太近（来不及预取）都无效。正确距离 ≈ cache miss latency / 每元素处理时间。""",
"""### 自测题

<details>
<summary>1. 存储器山展示的是什么关系？山脊和悬崖分别代表什么？</summary>

二维测试：读数组时**读吞吐（MB/s）**随 **stride**（步长）和 **working set size**（工作集大小）变化。**山脊**：stride 小 + working set < cache → 高吞吐（cache 命中）；**悬崖**：working set 超出 L3 → 吞吐骤降（DRAM miss）。亲眼见 stride=8 元素 vs 1 元素差一个数量级。
</details>

<details>
<summary>2. 分块（blocking/tiling）是什么？为什么能提高性能？</summary>

把大矩阵运算拆成小块，使每个子块 **fit L1/L2 cache**。例如矩阵乘法 C=A×B，把 A、B、C 分成 64×64 的子块，每个子块 64×64×8B=32KB fit L1。子块内的运算 cache 命中率高，总 miss 数大幅减少。**代价**：需要处理块边界（矩阵尺寸不是块大小整数倍）。
</details>

<details>
<summary>3. 循环融合（fusion）是什么？HFT 中什么时候用？</summary>

把多次遍历同一数据的循环**合并成一次**。例如 `for: sum += a[i]; for: max = max(max, a[i]);` 融合成 `for: { sum += a[i]; max = max(max, a[i]); }`——只扫一次数组，减少总 cache miss 和内存带宽消耗。HFT 中 tick 数据批量统计时常用。
</details>"""),

("section-6.7-小结.md",
"""### 常见陷阱

1. **学完 Ch6 就以为 cache 优化做完了** — Ch6 讲的是单核 cache 原理；多核场景还有 MESI 一致性协议、false sharing、NUMA 等问题（→ Hennessy Ch2/Ch5）。
2. **只优化数据 cache 忽略指令 cache** — 热函数太大/跳转分散导致 I-cache miss。用 `perf stat -e iCache-misses` 检查。
""",
"""### 自测题

<details>
<summary>1. Ch6 全章最核心的三个教训是什么？</summary>

①**局部性决定性能**——时间局部性（复用数据）+ 空间局部性（顺序访问）；②**cache line 是最小单位**——即使读 1 字节也拉 64B，布局要对齐 cache line；③**工作集要 fit cache**——超过容量就 capacity miss，用分块（blocking）控制。
</details>

<details>
<summary>2. HFT cache 优化的完整检查清单是什么？</summary>

①热数据是否连续存储（数组/vector/节点池）；②循环是否按行扫（stride=1）；③工作集是否 fit L1/L2；④结构体字段是否紧凑（热字段放前面）；⑤是否 false sharing（多线程写同一 cache line）；⑥是否每包 malloc（改预分配）；⑦`perf stat` 检查 `cache-misses`/`L1-dcache-load-misses`。
</details>"""),
]

count = 0
for filename, traps_text, quiz_text in SECTIONS:
    filepath = os.path.join(NOTES_DIR, filename)
    if not os.path.exists(filepath):
        print(f"SKIP (not found): {filename}")
        continue
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Skip if already has pitfalls/quiz
    if "### 常见陷阱" in content or "### 自测题" in content:
        print(f"SKIP (already has pitfalls/quiz): {filename}")
        continue

    block = traps_text + "\n\n" + quiz_text + "\n"

    # Pattern 1: replace empty quiz "### 口述巩固 · 自测\n\n1. （待口述补）..."
    pattern1 = r"### 口述巩固 · 自测\s*\n\s*1\. （待口述补）本节核心一句话？"
    if re.search(pattern1, content):
        content = re.sub(pattern1, block.rstrip(), content)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        count += 1
        print(f"REPLACED (empty quiz): {filename}")
        continue

    # Pattern 2: insert before trailing nav line "← [本章导读](../README.md)"
    # Find the LAST occurrence of "---\n\n← [本章导读]"
    nav_pattern = r"\n---\n\n← \[本章导读\]\(\.\./README\.md\)$"
    if re.search(nav_pattern, content):
        content = re.sub(nav_pattern, "\n---\n\n" + block + "\n---\n\n← [本章导读](../README.md)", content)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        count += 1
        print(f"INSERTED (before nav): {filename}")
        continue

    # Pattern 3: file ends with just "← [本章导读](../README.md)" without ---
    if content.rstrip().endswith("← [本章导读](../README.md)"):
        content = content.rstrip() + "\n\n---\n\n" + block + "\n\n---\n\n← [本章导读](../README.md)\n"
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        count += 1
        print(f"APPENDED (after content): {filename}")
        continue

    print(f"SKIP (no pattern matched): {filename}")

print(f"\nDone: {count}/{len(SECTIONS)} files processed")
