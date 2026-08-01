## 2. 流动性的维度 (Dimensions of Liquidity)

流动性是 **多维概念** — 讨论时须指明 **哪一维**（避免混淆）。

### 2.1 三大核心 + 一衍生

| 维度 | 英文 | 问什么 | 典型关注者 |
|------|------|--------|------------|
| **即时性** | Immediacy | 给定成本下，安排 **特定规模** 的 **速度** | 急躁小额 taker |
| **宽度** | Width | 给定规模下的 **成本** | 小额交易 |
| | | 体现：**买卖价差** + **佣金** | [Ch 14](../chapter-14-bid-ask-spreads/) |
| **深度** | Depth | 给定成本下能成交的 **规模** | **大资金** |
| **弹性** | Resiliency | 价格因 **不知情大单失衡** 偏离基本面后，**恢复的速度** | 市场质量 / 监管 |

→ 弹性来源 [Ch 16 价值交易者](../chapter-16-value-traders/)

### 2.2 维度间的权衡

| 想要… | 通常牺牲… |
|--------|-----------|
| **更大规模（深度）** | 更差价格（**宽度**）或更多时间（**即时性**） |
| **更快（即时性）** | 更宽 spread / 更大 impact |
| **更窄（宽度）** | 排队等待 / 不保证成交 |

```
        Immediacy
           /\
          /  \
         /    \
    Width ——— Depth
         \    /
          \  /
       Resiliency（偏离后恢复）
```

| HFT 视角 |
|----------|
| **BBO spread** → width；**book depth at N ticks** → depth |
| **Recovery time after shock** → resiliency（flash crash 后多久回 mid） |
| Go DEX M3：对同一 `orderbook` 测 **spread、累计深度、大单冲击后 mid 恢复** |

---
