# Chapter 10: DAMON

> 来源：Bootlin（DAMON lab）+ LWN（DAMON 设计）
> 对标：Mel Gorman（无 DAMON，6.x 新增）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [damon-lab](notes/01-damon-lab.md) | Bootlin：DAMON 操作实验、damo CLI、scheme 配置 |
| 2 | [damon](notes/02-damon.md) | LWN：DAMON 数据结构、访问跟踪、region 划分 |

## HFT 关联

- **DAMON 访问模式分析**：HFT 可用 DAMON 监控进程的内存访问热点，识别冷页用于 pre-demotion
- **DAMOS scheme**：DAMON Action Scheme 可自动回收冷页，比 kswapd 更精准
- **热页识别**：DAMON 的采样结果可直接指导 `mlock` 策略——只钉住真正热的页面
- **低开销**：DAMON 的采样器开销 < 1%，可长期运行在生产环境

## 交叉引用

- `06.5-modern-mm/chapter-07-page-reclaim-mglru/`：DAMON 与 MGLRU 协同
- `15-bpf-observability/`：eBPF 也可做访问跟踪，DAMON 是内核原生方案
