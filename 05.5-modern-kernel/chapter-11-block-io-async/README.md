# Ch11 块 I/O 与异步 I/O

> 来源: LWN.net
> 对标旧书: ULK3 Ch14 (单队列已过时)

blk-mq 多队列、io_uring 异步 I/O 框架。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 11.1 blk-mq 多队列 (LWN) | `notes/01-blk-mq.md` |
| 11.2 io_uring 异步 I/O (LWN) | `notes/02-io-uring.md` |

---

## HFT 关联

HFT 磁盘 I/O 主要用于日志和行情回放。blk-mq 减少锁争用，io_uring 提供高吞吐异步 I/O。
