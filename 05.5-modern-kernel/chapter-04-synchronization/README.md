# Ch4 同步原语 (qspinlock)

> 来源: LWN.net
> 对标旧书: ULK3 Ch5 (ticket spinlock 已过时)

Ticket spinlock 的缓存弹跳问题、qspinlock 的 MCS 队列设计。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 4.1 Ticket Spinlock 问题与缓存弹跳 | `notes/01-ticket-spinlock-problem.md` |
| 4.2 qspinlock 设计：MCS 队列与三级优化 | `notes/02-qspinlock-design.md` |

---

## HFT 关联

qspinlock 减少高争用场景下的缓存行弹跳，间接降低交易线程的缓存干扰。
