## ④ 数据对齐和结构体填充

#### 自然对齐 · Natural Alignment

| 规则 | 变量地址必须是 **其类型大小的整数倍** |
|------|--------------------------------------|
| 未对齐访问 | 某些架构 **异常** · 其他架构 **严重变慢** |

#### 结构体填充 · Padding

| 原因 | 编译器在成员间插 **填充字节** 以满足对齐 |
|------|------------------------------------------|
| 优化 | **调整成员顺序** — 把大对齐放前，减空洞 |

| 陷阱 | 结构体 **原样** 发网络/写盘 — **不同架构 padding 不同** → **不兼容** |

```c
/* 协议/on-wire 布局：用 packed 或显式 u8 数组 — 慎用手搓 */
struct hdr {
    __be32 magic;
    __be16 len;
} __packed;
```

**HFT：** 交易所报文 **固定布局 + ntoh/hton** — 与内核 **`cpu_to_be32`** 同一纪律。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 结构体对齐规则是什么？如何强制 packed？有什么代价？

<details><summary>答案</summary>

默认对齐：成员按自身大小对齐（int 4 字节对齐），结构体大小 = 最大成员对齐的倍数。`__attribute__((packed))` 取消填充，紧凑排列。代价：1) 非对齐访问在某些架构（ARM）触发异常或变慢（x86 自动处理但有性能损失）；2) 不能用 atomic 操作。HFT 网络协议解析用 packed 结构体映射报文头，性能关键路径用手动偏移。

</details>

**Q2.** 为什么 cache line 对齐对 HFT 至关重要？

<details><summary>答案</summary>

CPU cache line 64 字节。如果两个变量在同一 cache line 且分别被不同 CPU 核频繁写 → false sharing（缓存行 bouncing）。`__attribute__((aligned(64)))` 强制变量独占 cache line。HFT 交易线程和风控线程各自计数器必须 cache line 对齐，否则每秒万次写 → L1 cache 反复 invalidate → 性能下降 10x+。

</details>

</details>
---
