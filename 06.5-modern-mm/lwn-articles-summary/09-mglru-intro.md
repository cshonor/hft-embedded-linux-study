# MGLRU 简介

> **原文:** [MGLRU: Multi-Gen LRU](https://lwn.net/Articles/856831/) (LWN, 2022)
> **作者:** Yu Zhao (Google)
> **内核版本:** 6.1+ (合入主线)
> **对标旧书:** ULK3 Ch17 (传统 LRU 已过时)

---

## 核心观点

MGLRU (Multi-Generational LRU) 是 Google 开发的新 LRU 实现，通过多代分级提高页回收精度和效率。

### 传统 LRU 的不足

```
传统 LRU (2 代):
  Active ←→ Inactive
  
问题:
  1. 只有 2 代，精度粗
  2. inactive 链表很长，扫描慢
  3. 工作集在 active↔inactive 反复迁移 (thrashing)
  4. 大内存机器回收扫描耗时长
```

### MGLRU 设计

```
MGLRU (N 代):
  最年轻代 (newest) → ... → 最老代 (oldest)
  
  新页 → 进入最年轻代
  页被访问 → 晋升到更年轻的代
  回收 → 从最老代开始

  代分为两类:
    Gen 0, 2, 4, ... (youngest set): 活跃代
    Gen 1, 3, 5, ... (oldest set):   不活跃代

  晋升: oldest set → youngest set (批量)
  回收: oldest set 尾部页
```

### 关键优势

| 指标 | 传统 LRU | MGLRU |
|------|---------|-------|
| 代数 | 2 | N (默认 max 4-8) |
| 页扫描量 | 全 inactive 链表 | 仅最老代 (批量小) |
| 工作集保护 | 一般 | 老代不被轻易回收 |
| cold memory 检测 | 粗 | 精细 (按代时间戳) |
| CPU 占用 | 高 (大内存) | 低 (减少 90% 扫描) |

### 使用方法

```bash
# 启用 MGLRU (6.1+)
echo y > /sys/kernel/mm/lru_gen/enabled
# 或
echo y > /sys/kernel/mm/lru_gen/enabled && echo y > /sys/kernel/mm/lru_gen/min_ttl_ms

# 查看 MGLRU 状态
cat /sys/kernel/mm/lru_gen/debugfs  # 需要 CONFIG_LRU_GEN_DEBUG
```

---

## 与旧书差异

| ULK3 讲的 | MGLRU |
|-----------|-------|
| active/inactive 二分 | 多代分级 |
| 全链表扫描 | 按代批量扫描 |
| `shrink_active_list()` | `lru_gen_look_around()` |

---

## HFT 关联

MGLRU 对 HFT 直接影响小（HFT 禁 swap + mlockall），但减少 kswapd CPU 占用有益于整体系统稳定性。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** MGLRU 如何减少 90% 的页扫描？

> 传统 LRU 回收时从整个 inactive 链表尾部扫描，大量页需要检查 referenced 标志。MGLRU 只扫描最老代的页，而最老代只包含最久未访问的页（数量远少于全 inactive）。加上按代批量处理，单次回收扫描的页数减少 90%。

**Q2:** MGLRU 的 "generation" 和传统 LRU 的 "active/inactive" 有什么本质区别？

> active/inactive 是二元分类，只有"活跃"和"不活跃"两个状态。generation 是时间序列分级，页按最后访问时间分到不同代。代数越多，分级越精细。例如 4 代可以区分"1秒前访问"、"10秒前访问"、"1分钟前访问"、"10分钟前访问"，而传统 LRU 只能区分"活跃"和"不活跃"。

</details>
