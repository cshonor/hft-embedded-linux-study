# Chapter 08: OOM/PSI/zswap

> 来源：Bootlin（OOM/PSI）+ LWN（PSI 压力 + zswap）
> 对标：Mel Gorman（无 PSI，OOM 仅基础版）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [oom-psi](notes/01-oom-psi.md) | Bootlin：OOM killer 流程、oom_score_adj、PSI 基础 |
| 2 | [psi-pressure](notes/02-psi-pressure.md) | LWN：PSI 设计、cpu/memory/io 压力、poll 模式 |
| 3 | [zswap](notes/03-zswap.md) | LWN：zswap 压缩前端、pool 限制、zsmalloc vs zbud |

## HFT 关联

- **PSI 监控**：HFT 进程可 poll `/proc/pressure/memory` 获取微秒级内存压力，提前告警
- **OOM killer**：HFT 进程应设 `oom_score_adj=-1000` 避免 被 kill；交易进程绝不能被 OOM
- **zswap 压缩延迟**：zswap 压缩/解压引入 CPU 开销和延迟抖动，HFT 机器应禁用 zswap（`echo 0 > /sys/module/zswap/parameters/enabled`）
- **PSI 阈值**：可设 PSI trigger 在 pressure 超阈值时唤醒监控线程，实现主动防御

## 交叉引用

- `06.5-modern-mm/chapter-07-page-reclaim-mglru/`：回收触发 PSI 压力
- `16-systems-performance/`：系统级性能监控
