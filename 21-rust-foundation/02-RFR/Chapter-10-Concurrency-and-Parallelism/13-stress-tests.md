# 5.2 Write Stress Tests（压力测试）

> 所属：**Sane Concurrency** · [← 章索引](./README.md)

多线程**高并发读写**「抖动」出：

- 竞态
- 死锁
- 逻辑顺序 bug

## 技巧

- 随机化调度 / 短临界区 / 大量迭代
- 结合 [14 工具](./14-concurrency-testing-tools.md)

ER → [Item 30 超越单元测试](../../01-ER/Chapter-05-Tooling/Item-30-beyond-unit-tests/README.md)
