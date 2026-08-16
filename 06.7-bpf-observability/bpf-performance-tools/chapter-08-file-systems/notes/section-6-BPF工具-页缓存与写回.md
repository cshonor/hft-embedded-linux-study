# 8.3 BPF 工具：页缓存与写回（8.3.16–8.3.17）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节（印刷 p331–336）

| 工具 | 来源 | 一句话 |
|---|---|---|
| cachestat | BCC | 页缓存命中率/脏页每秒统计（cachetop 为按进程 curses 版） |
| writeback | BT | 写回事件：时间、设备、页数、原因、耗时 |

## cachestat —— 页缓存命中率 🔴

```
# cachestat -T
TIME        HITS   MISSES  DIRTIES  HITRATIO  BUFFERS_MB  CACHED_MB
21:09:08    33036  53975   400      37.97%    15          ...
21:10:08    268421 12      784      100.00%   ...
```

**为什么 90%→100% 的收益远大于 10%**：剩下的 10% 未命中意味着应用仍要等磁盘 I/O；全命中后应用完全跑在内存里，尾延迟天壤之别。

**书中的验证实验（值得背下来）**：

1. 闲置系统创建 1GB 文件 → 观察到 DIRTIES 攒页、CACHED_MB 涨 1024MB；
2. `sync; echo 3 > /proc/sys/vm/drop_caches` 清缓存；
3. 第一次读该文件 → HITRATIO 掉到 ~38%，MISSES 高企（冷读全落盘）；
4. 第二次读 → HITRATIO 100%（纯页缓存命中）。

生产应用：Cassandra/Elasticsearch/PostgreSQL 等有状态服务靠页缓存保活跃数据集常驻内存。Netflix 用 cachestat 回答容量规划核心问题——**最活跃数据集是否装得进内存**、加内存是否值得、压缩算法选型。

实现（kprobe 四函数，强依赖内核版本）：

| 函数 | 测量 |
|---|---|
| mark_page_accessed() | 缓存访问（命中） |
| mark_buffer_dirty() | 缓存写入 |
| add_to_page_cache_lru() | 页添加（未命中） |
| account_page_dirtied() | 脏页 |

- 开销：这些函数每秒可达百万次调用，极端情况 **>30%**；生产使用前必须先在测试环境验证。作者自述该工具"像沙堡城堡"（sandcastle），新内核很容易打破。
- 变体：cachetop(8)（BCC，curses 按进程版）。
- 健壮化路线（作者建议）：A) 花数周学内核源码并与内存管理开发者合作；B) 内核加 /proc 统计，工具只读计数器。

## writeback —— 脏页何时、为何、多慢地落盘

```
# writeback.bt
TIME      DEVICE  PAGES  REASON     ms
03:42:55  253:1   40     periodic   0.167   ← 每5秒周期性写回（页少）
03:43:01  253:1   11268  background  6.112   ← 空闲内存低触发的后台写回（万页级，6–22ms）
03:43:04  253:1   38836  sync       64.655   ← 同步写回一次性 38836 页
```

分析姿势：若 background 写回的时间点与应用延迟尖峰（云监控记录）吻合 ⇒ 应用被写回 I/O 挤压。可调 `vm.dirty_writeback_centisecs` 等 sysctl。

实现：tracepoint `writeback:writeback_start` 记开始时间与 nr_pages（按 sb_dev 键），`writeback:writeback_written` 算差。REASON 映射表：0=background, 1=vmscan, 2=sync, 3=periodic, 4=laptop_timer, 5=free_more_memory, 6=fs_free_space, 7=fs_work。页数取 start 与 written 的 nr_pages 差值（与内核 wb_writeback 统计一致，见 fs/fs-writeback.c）。

## HFT 关联

- 开盘前预加载行情/参考数据 → 用 cachestat 验证 HITRATIO 到 100%，避免盘中冷读。
- 写回风暴防御：交易日志盘的 dirty 参数（dirty_ratio/dirty_writeback_centisecs）调优 + writeback.bt 监控 background 写回时段是否撞上交易高峰。

## 常见陷阱

- cachestat 基于内核内部函数，换内核先在测试机验证，别直接上生产。
- drop_caches 实验本身是破坏性操作（清空全系统页缓存），只在实验环境做。

<details>
<summary>自测</summary>

1. cachestat 的四个 kprobe 各测什么？为什么它跨内核脆弱？
2. writeback 输出中 background 与 periodic 写回的触发条件差异？
3. 命中率从 90% 提到 100% 为什么收益远超 10%？
</details>
