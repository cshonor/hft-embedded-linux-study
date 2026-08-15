# Chapter 07: 页回收与 LRU/MGLRU

> 来源：笨叔卷1 + Bootlin + LWN（LRU → MGLRU 全系列）
> 对标：Mel Gorman Ch9/10（LRU → MGLRU）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [page-reclaim-ben-shu](notes/01-page-reclaim-ben-shu.md) | 笨叔：页回收基础、kswapd、watermark、direct reclaim |
| 2 | [page-reclaim-bootlin](notes/02-page-reclaim-bootlin.md) | Bootlin：回收路径、file/anon LRU、swappiness |
| 3 | [lru-basics](notes/03-lru-basics.md) | LWN：LRU 链表实现、active/inactive、pagevec 批量 |
| 4 | [mglru-intro](notes/04-mglru-intro.md) | LWN：MGLRU 设计动机、generational 分代、游标替代旋转 |
| 5 | [mglru-merge](notes/05-mglru-merge.md) | LWN：MGLRU 合并 6.1、默认开启、性能数据 |

## HFT 关联

- **direct reclaim 延迟**：direct reclaim 在分配路径同步回收，可导致毫秒级阻塞；HFT 必须用 `mlockall` 或 cgroup 限制避免
- **MGLRU 优势**：MGLRU 减少 LRU 链表锁争用，分代回收更精确，减少误回收热页
- **swappiness=0**：HFT 机器应设 `vm.swappiness=0`，优先回收 file page 而非 swap anon
- **watermark**：`min/low/high` watermark 控制 kswapd 唤醒，HFT 应调高 watermark 避免紧急回收

## 交叉引用

- `06.5-modern-mm/chapter-06-page-cache-folio/`：folio 在 LRU 上的管理
- `06.5-modern-mm/chapter-08-oom-psi-zswap/`：PSI 监控回收压力
