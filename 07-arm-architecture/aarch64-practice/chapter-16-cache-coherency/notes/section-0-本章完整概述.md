# Ch16 完整总结 · 缓存一致性

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

多核 Cache 一致性协议（MESI）、伪共享、DMA 一致性。选读——多核 SoC 场景重要。

---

## 16.1 MESI 协议 ⭐

四个状态首字母：**M**odified、**E**xclusive、**S**hared、**I**nvalid

| 状态 | 含义 | Cache vs 内存 | 其他核有副本？ |
|------|------|--------------|---------------|
| **M** | 已修改 | Cache 有最新数据，内存过期 | 无 |
| **E** | 独占 | Cache = 内存 | 无 |
| **S** | 共享 | Cache = 内存 | 有（也是 S） |
| **I** | 无效 | Cache 行无效 | — |

### 状态转换

```
读 miss → 从内存加载 → E（如果无其他核）或 S（如果其他核有）

写 S → 广播 Invalidate → 其他核变 I → 本核变 M

写 E → 直接改 → M（无需广播）

M 被逐出 → 写回内存 → E（如果独占）或 S
```

| 操作 | M | E | S | I |
|------|---|---|---|---|
| 读命中 | M | E | S | 从内存读→E/S |
| 写命中 | M | M→M | S→M（广播Inv） | 从内存读→M |
| 读miss | 写回→S | E→S | — | 读→E/S |
| 写miss | 写回→M | E→M | S→M | 读→M |

---

## 16.2 伪共享（False Sharing）⭐

```
// 两个变量在同一 Cache Line（64字节）内
struct {
    int a;    // offset 0，CPU0 频繁写
    int b;    // offset 4，CPU1 频繁写
    // padding... 共 64 字节
} data;
```

- CPU0 写 `a` → Cache 行变 M → CPU1 的同一行变 I
- CPU1 写 `b` → 要重新加载 → 变 M → CPU0 的行变 I
- 循环往复 → Cache 行在两核间反复搬运 → 性能暴跌

**修复：** 对齐填充让不同核访问的变量不在同一 Cache Line。

```c
// 用 __cacheline_aligned 或手动 padding
struct {
    int a;
    char pad[60];   // 填充到 64 字节
    int b;
} data;

// 或用 GCC 属性
struct data {
    int a __attribute__((aligned(64)));
    int b __attribute__((aligned(64)));
};
```

---

## 16.3 DMA 一致性

| 场景 | 问题 | 解决 |
|------|------|------|
| DMA→内存 | CPU Cache 有旧数据 | 先 **invalidate** |
| 内存→DMA | CPU Cache 有新数据未写回 | 先 **clean** |
| 自修改代码 | I-Cache 有旧指令 | 先 clean D-cache → invalidate I-cache |

```c
// Linux DMA 映射 API
dma_map_single(dev, addr, size, DMA_FROM_DEVICE);  // 设备→内存：invalidate
dma_map_single(dev, addr, size, DMA_TO_DEVICE);    // 内存→设备：clean
dma_unmap_single(dev, addr, size, direction);       // 完成后恢复
```

> 如果硬件支持 **IOMMU/SMMU**，可能自动维护一致性，不需要软件 flush。

---

## 16.4 自修改代码

```c
// 修改代码后必须刷新 I-Cache
void flush_icache_range(unsigned long start, unsigned long end) {
    // 1. Clean D-cache（写回修改的指令）
    // 2. DSB（确保写回完成）
    // 3. Invalidate I-cache（丢弃旧指令缓存）
    // 4. DSB + ISB（确保后续取指用新指令）
}
```

> JIT 编译器、内核 module 加载、kprobes 都需要处理 I-Cache 一致性。

---

## 16.5 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 16-1 | 高速缓存伪共享（性能对比） | Linux |
| 16-2 | 使用 Perf C2C 发现伪共享 | Linux |

---

## 16.6 易错点清单

1. **伪共享** → 不同核的变量在同一 Cache Line，反复 invalidate。
2. **DMA 不做 Cache 操作** → 数据不一致。
3. **自修改代码不刷 I-Cache** → 执行旧指令。
4. **MESI 记混** → M=修改（内存过期）；E=独占（=内存）；S=共享（=内存，多核有副本）。

---

## 书中思考题（自测）

1. MESI 四个状态分别是什么？M 和 E 的区别？
2. 什么是伪共享？怎么修复？
3. DMA 从设备读数据到内存，CPU 应该先做什么 Cache 操作？
4. 自修改代码为什么要刷 I-Cache？
5. S 状态的 Cache 行被写时发生什么？

**参考答案：**

1. M=已修改(内存过期)；E=独存 Cache=内存)；S=共享(=内存，他核也有)；I=无效。M vs E：M 的**内存已过期**，E 的**内存是最新的**。  
2. 不同核访问的变量在同一 Cache Line → 反复 invalidate。修复：**对齐填充**到不同 Cache Line。  
3. 先 **invalidate** CPU Cache（丢弃旧值，让 CPU 之后从内存读新值）。  
4. D-cache 新指令写回后，I-cache 还缓存旧指令 → 必须 invalidate I-cache。  
5. 广播 **Invalidate** 给其他核 → 其他核变 I → 本核变 M。

---

上一章 [Ch15 Cache基础](../../chapter-15-cache-basics/) · 下一章 [Ch17 TLB管理](../../chapter-17-tlb-management/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
