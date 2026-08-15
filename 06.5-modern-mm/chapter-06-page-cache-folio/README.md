# Chapter 06: 页缓存与 Folio

> 来源：笨叔卷1 + Bootlin + LWN（folio 全系列）
> 对标：Mel Gorman Ch8（page cache → folio）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [page-cache-folio-ben-shu](notes/01-page-cache-folio-ben-shu.md) | 笨叔：page cache 基础、writeback、脏页管理 |
| 2 | [page-cache-folio-api](notes/02-page-cache-folio-api.md) | Bootlin：folio API、folio_mkclean、address_space 操作 |
| 3 | [folio-proposal](notes/03-folio-proposal.md) | LWN：folio 提案动机、page 结构体问题、tail page 浪费 |
| 4 | [folio-deep-dive](notes/04-folio-deep-dive.md) | LWN：folio 实现细节、compound page、LRU 管理 |
| 5 | [folio-merge-status](notes/05-folio-merge-status.md) | LWN：folio 合并进度、6.x 内核采用率、剩余工作 |

## HFT 关联

- **O_DIRECT 绕过 page cache**：HFT 网络收发不经过 page cache，但磁盘 I/O 日志记录可能用到；folio 减少元数据开销
- **folio 减少 page 结构体**：一个 2MB folio 替代 512 个 page 结构体，节省 32KB 内存 + 减少 LRU 遍历
- **writeback 延迟**：folio 的批量 writeback 比 per-page 减少 I/O 提交次数，降低抖动
- **compound page**：folio 底层是 compound page，与 hugepage 共享基础设施

## 交叉引用

- `06.5-modern-mm/chapter-07-page-reclaim-mglru/`：folio 在 LRU 上的管理
- `06-linux-mm/`：Mel Gorman page cache 实现（已过时）
