# Ch3 RCU 现代实现

> 来源: 笨叔《奔跑吧Linux内核》 + LWN.net
> 对标旧书: ULK3 Ch5 (RCU 已大幅重构)

RCU 基础原理、Tree RCU、gp/cbs 分离、rcu_read_lock 变化。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 3.1 RCU 基础 (LWN) | `notes/01-rcu-basics.md` |
| 3.2 RCU 进阶 (LWN) | `notes/02-rcu-advanced.md` |
| 3.3 RCU 笔记 (笨叔) | `notes/03-rcu-ben-shu.md` |

---

## HFT 关联

RCU 是内核中最重要的无锁读取机制。HFT 交易线程读取内核数据（如路由表）走 RCU 路径，不受写者干扰。
