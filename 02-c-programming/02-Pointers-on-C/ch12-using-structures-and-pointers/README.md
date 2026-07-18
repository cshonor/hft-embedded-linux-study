# 第 12 章 使用结构和指针

**Using Structures and Pointers**

## 本章讲什么

**堆 struct + 指针** 综合实战：单向/双向链表、**`struct **`** 改头、嵌套堆成员分层释放、结构体指针数组。DPDK mbuf 链、内核 list、HFT 订单池的代码模板本章。

## 学习重点

- **calloc** 建节点；判 NULL
- 头插 **`list_push(Quote **head, ...)`**
- 销毁：**tmp = cur->next; free(cur)**
- **`list_remove_by_seq`**；删头用二级指针
- 嵌套 **`Msg`**：先 free 子指针再 free 外层
- **`Quote *arr[]`** vs **`Quote arr[]`**（批量收包）
- 双向链：**prev/next**；内核 list_head 思想

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mbuf 链、`rte_mbuf *pkts[]`、批量释放 |
| 内核 | 双向 list_head、设备/进程链 |
| HFT | 订单链头插、撤单删除、指针数组批处理 |

## 线上陷阱（汇总）

1. 漏 free 嵌套堆成员  
2. free 后读 `next`  
3. 删头传一级指针  
4. 栈 struct 内堆指针悬垂  
5. malloc 未清零野 `next`  

## 实操（建议完成）

1. 单向链：头插 / 按 seq 删 / 全销毁  
2. **Msg** 分层 free + valgrind  
3. 模拟 `rte_rx_burst` 指针数组解析  
4. 实体数组 vs 指针数组拷贝对比  
5. 错误 free 顺序复现崩溃  
6. 二级指针删头  
7. 双链表插入与 O(1) 摘除  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 二级指针；ch10 自引用；ch11 堆分配 |
| 后序 | ch13 高级指针；ch17 ADT |
| 配套 | 《C陷阱与缺陷》ch03、ch05 |

## 小节

- [12.1 链表](./12.1-链表.md)
- [12.2 单链表](./12.2-singly-linked-lists/12.2-singly-linked-lists.md)
- [12.3 双链表](./12.3-doubly-linked-lists/12.3-doubly-linked-lists.md)
