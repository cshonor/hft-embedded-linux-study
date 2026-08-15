# 2.2 Worker Pools（工作池）

> 所属：**Concurrency Models** · [← 章索引](./README.md)

固定 worker 从队列取任务 — **rayon**、Tokio worker 等。

## 工作窃取 (work stealing)

空闲 worker 从伙伴队列偷任务 → 提高利用率。

## 适合

同质任务、**数据并行**（大数组分块）。

Book → [16.1 线程](../../00-Book/16-fearless-concurrency/16.1-使用线程同时运行代码.md)
