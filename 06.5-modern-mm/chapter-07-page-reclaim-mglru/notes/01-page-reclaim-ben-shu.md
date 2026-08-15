# 页回收 (LRU → MGLRU)

> 笼叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

### 页回收机制概览

当内存不足时，内核通过 LRU (Least Recently Used) 算法回收不活跃的页。回收的页包括：clean 页缓存（直接丢弃）、dirty 页缓存（写回磁盘后丢弃）、匿名页（写入 swap 后丢弃）。

### 传统 LRU（2.6 ~ 5.x）

```
每个 zone 有 5 个 LRU 链表:
  inactive_anon   — 不活跃匿名页
  active_anon     — 活跃匿名页
  inactive_file   — 不活跃文件页 (页缓存)
  active_file     — 活跃文件页
  unevictable     — 不可回收页 (mlock 等)

回收流程:
  1. kswapd 内核线程被唤醒 (free < watermark)
  2. 从 inactive LRU 尾部扫描
  3. 如果是 clean file page → 直接丢弃
  4. 如果是 dirty file page → 写回后丢弃
  5. 如果是 anon page → 写入 swap 后丢弃
```

### MGLRU (Multi-Gen LRU, 6.1+)

MGLRU 是 Google 开发的新 LRU 实现，合入 Linux 6.1：

| 特性 | 传统 LRU | MGLRU |
|------|---------|-------|
| 代数 | 2 代 (active/inactive) | N 代 (多代) |
| 精度 | 粗粒度 (active↔inactive) | 细粒度 (按访问时间分多代) |
| 扫描效率 | 全链表扫描 | 按代批量扫描，更精准 |
| 工作集保护 | 一般 | 更好 (老代不轻易被回收) |
| 性能 | 大内存机器回收慢 | 显著改善 (减少 90% 页扫描) |

**MGLRU 核心思想：** 不是简单的 active/inactive 二分，而是维护多个"代"（generation），每代记录一批页。新页进入最年轻代，回收从最老代开始。页被访问时"晋升"到更年轻的代。

```bash
# 启用 MGLRU (6.1+)
echo y > /sys/kernel/mm/lru_gen/enabled

# 查看代信息
cat /sys/kernel/mm/lru_gen/debugfs  # 需要 CONFIG_LRU_GEN_DEBUG=y
```

### swap 与 zswap

```bash
# zswap: 压缩的 swap 缓存 (内存中)
# 启用 zswap
echo 1 > /sys/module/zswap/parameters/enabled
echo zstd > /sys/module/zswap/parameters/compressor  # 压缩算法
echo 20 > /sys/module/zswap/parameters/max_pool_percent  # 最多用 20% 内存

# zram: 压缩的 RAM 块设备
modprobe zram num_devices=1
echo lz4 > /sys/block/zram0/comp_algorithm
echo 2G > /sys/block/zram0/disksize
mkswap /dev/zram0
swapon /dev/zram0
```

### OOM Killer

```c
// 内存耗尽时，OOM killer 选择进程杀死
// 选择标准: oom_score (基于 RSS + nice + 运行时间)
// /proc/<pid>/oom_score_adj 可调整 (-1000 到 1000)
echo -1000 > /proc/<pid>/oom_score_adj  // 永不被 OOM
echo 1000 > /proc/<pid>/oom_score_adj   // 优先被 OOM
```

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| 2 代 LRU (active/inactive) | MGLRU 多代 LRU (6.1+) | 大内存机器回收效率 |
| per-zone LRU | per-node LRU + memcg LRU | NUMA 和 cgroup 支持 |
| 无 zswap | zswap (3.11+) / zram | 压缩比 swap 快 |
| `badness()` OOM 评分 | `oom_badness()` + oom_score_adj | 更灵活的 OOM 控制 |
| kswapd 单线程 | kswapd + 多线程回收 | 大内存并行回收 |

---

## 关键数据结构 / 函数

```c
// 源码路径: mm/vmscan.c
// MGLRU 核心结构
struct lruvec {
    struct lru_gen_struct lrugen;  // MGLRU 代信息
    // ...
};

struct lru_gen_struct {
    unsigned long max_seq;         // 最年轻代号
    unsigned long min_seq[ANON_AND_FILE];  // 最老代号 (匿名/文件分开)
    struct list_head folios[MAX_NR_GENS][ANON_AND_FILE][MAX_NR_ZONES];
    // folios[代号][类型][zone] — 按代分组的 folio 链表
};

// 源码路径: mm/oom_kill.c
int oom_badness(struct task_struct *p, unsigned long totalpages);
```

---

## HFT 关联

- **禁用 swap**：HFT 系统 `swapoff -a` 禁用 swap，任何页回收导致 swap I/O 都是毫秒级灾难
- **mlockall 防回收**：锁定所有交易相关页在内存，不会被 kswapd 回收
- **MGLRU 对 HFT 影响**：HFT 内存使用模式简单（大页池 + 少量堆），MGLRU 的多代精度对 HFT 帮助不大，但减少 kswapd 扫描开销有益
- **OOM 保护**：HFT 交易进程设 `oom_score_adj = -1000`，确保永不被 OOM killer 选中
- **zswap 不适用于 HFT**：压缩/解压缩引入微秒级延迟，HFT 应直接禁用 swap + zswap

---

## 自测

<details>
<summary>Q1: MGLRU 相比传统 LRU 的核心改进是什么？为什么对大内存机器效果显著？</summary>

核心改进是将 2 代 (active/inactive) 扩展为 N 代，按页访问时间精细分级。传统 LRU 在大内存机器上问题：(1) inactive 链表很长，扫描耗时长；(2) active↔inactive 的二分精度不够，容易误回收工作集页。MGLRU 按代批量扫描，老代才回收，工作集保护更好。Google 测试 Chrome OS 上减少 90% 页扫描，CPU 占用降低 20%。
</details>

<details>
<summary>Q2: HFT 系统应该如何配置内存回收相关参数？</summary>

(1) `swapoff -a` 禁用 swap；(2) `mlockall(MCL_CURRENT|MCL_FUTURE)` 锁定所有页；(3) `echo -1000 > /proc/<pid>/oom_score_adj` 防 OOM；(4) 减少 `vm.swappiness`（虽已禁 swap 但保持 `echo 0 > /proc/sys/vm/swappiness`）；(5) 预留大页池 `echo 1024 > /proc/sys/vm/nr_hugepages`；(6) 可关闭 transparent hugepage (`echo never > ...enabled`) 避免 khugepaged 抢占。
</details>

<details>
<summary>Q3: zswap 和 zram 的区别？为什么都不适合 HFT？</summary>

zswap 是内核内建的压缩 swap 缓存，在内存中压缩匿名页，需要时解压回内存，避免真正写入磁盘 swap。zram 是一个压缩的 RAM 块设备，需要配置为 swap 设备使用。两者都不适合 HFT：(1) 压缩/解压缩引入微秒级延迟（zstd 压缩 ~3μs/4KB）；(2) HFT 要求确定性延迟，压缩时间不稳定；(3) HFT 直接禁 swap + mlockall，不需要任何形式的 swap。
</details>
