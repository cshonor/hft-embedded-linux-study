# zswap

> **原文:** [zswap: Compressed swap cache](https://lwn.net/Articles/537422/) (LWN, 2013)
> **内核版本:** 3.11+
> **对标旧书:** 无 (ULK3/LKD3 未涉及)

---

## 核心观点

zswap 是内核内建的压缩 swap 缓存，在内存中压缩匿名页，避免直接写入磁盘 swap 设备。

### 工作原理

```
传统 swap 路径:
  anon page → 写入磁盘 swap 分区 → 磁盘 I/O (毫秒级)

zswap 路径:
  anon page → 压缩 (zstd/lz4) → 存在内存的 zswap pool → 命中时解压
  如果 zswap pool 满了 → LRU 淘汰 → 写入磁盘 swap (fallback)
```

### 配置

```bash
# 启用 zswap
echo 1 > /sys/module/zswap/parameters/enabled

# 压缩算法
echo zstd > /sys/module/zswap/parameters/compressor
# 可选: zstd (默认, 压缩比高), lz4 (速度快), deflate

# zswap 池最大占内存百分比
echo 20 > /sys/module/zswap/parameters/max_pool_percent  # 20%

# zpool 类型
echo zbud > /sys/module/zswap/parameters/zpool
# zbud: 最多 2 个压缩页/page, 碎片低但压缩比低
# z3fold: 最多 3 个/page, 平衡
# zsmalloc: 高密度, 碎片高但压缩比高
```

### zswap vs zram vs 传统 swap

| 特性 | zswap | zram | 传统 swap |
|------|-------|------|----------|
| 类型 | 内存中压缩缓存 | 压缩 RAM 块设备 | 磁盘分区/文件 |
| 需要后端 | 是 (磁盘 swap) | 否 | - |
| 压缩 | 内存中 | 内存中 | 无 |
| 延迟 | 微秒级 (命中) | 微秒级 | 毫秒级 (I/O) |
| 用途 | 通用服务器 | 嵌入式/移动 | 所有系统 |

---

## 与旧书差异

| ULK3 / LKD3 | 现代实现 |
|-------------|---------|
| 只有传统 swap | zswap (3.11+), zram |
| swap = 磁盘 I/O | zswap 在内存中压缩, 减少磁盘 I/O |

---

## HFT 关联

zswap **不适合 HFT**：(1) 压缩/解压缩引入微秒级延迟（zstd ~3μs/4KB）；(2) HFT 直接禁 swap + mlockall，不需要任何 swap 机制；(3) zswap 占用 CPU 进行压缩，影响交易线程。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** zswap 和 zram 的主要区别是什么？

> zswap 是磁盘 swap 的前置压缩缓存——需要磁盘 swap 作为后端，压缩页先在内存中，满了才写磁盘。zram 是一个独立的压缩 RAM 块设备——不需要磁盘后端，所有 swap 都在内存中完成。zswap 适合有磁盘 swap 的服务器，zram 适合无磁盘的嵌入式/移动设备。

**Q2:** 为什么 zswap 不适合 HFT？

> (1) 压缩延迟：zstd 压缩 4KB 约 3μs，解压 1μs，对纳秒级延迟要求的 HFT 不可接受；(2) CPU 占用：压缩消耗 CPU 周期，可能影响交易线程；(3) 不确定性：压缩时间取决于数据内容，引入尾延迟。HFT 应禁 swap + mlockall，确保所有页在物理内存中。

</details>
