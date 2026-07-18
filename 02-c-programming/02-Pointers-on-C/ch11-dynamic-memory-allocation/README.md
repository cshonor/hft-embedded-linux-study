# 第 11 章 动态内存分配

**Dynamic Memory Allocation**

## 本章讲什么

**堆 heap** 上 **malloc / calloc / realloc / free** 的生命周期与故障模式。栈/静态段无法承载变长、海量数据；DPDK mempool 是本章堆模型的工业优化版。

## 学习重点

- 栈 / 静态 / 堆三分区与选型
- **malloc** 不清零；**calloc** 清零；**realloc** 用临时指针
- 分配判 **NULL**；free 原始地址；**free 后 = NULL**
- 四大故障：泄漏、悬垂、双重 free、越界
- 碎片 → 内存池、固定块（**rte_mempool**）
- **posix_memalign**；多线程 malloc 锁竞争

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mempool 预分配、无锁、大页对齐 |
| 内核 | kmalloc / slab 对照 |
| HFT | 热路径禁频繁 malloc；订单/行情池 |

## 线上陷阱（汇总）

1. 未判 NULL  
2. realloc 直接赋回原指针  
3. 双重 free  
4. free 后继续使用  
5. 堆越界  
6. 异常分支泄漏  
7. 热路径 libc malloc 锁抖动  

## 实操（建议完成）

1. malloc vs calloc 脏数据  
2. realloc 丢指针 vs 安全写法  
3. 双重 free → SIGABRT  
4. 堆链表完整 free  
5. 堆越界观察损坏  
6. **posix_memalign** 64B  
7. valgrind 查泄漏  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 void*；ch08 VLA；ch10 堆 struct |
| 后序 | ch12 指针链表；ch17 ADT；ch18 mmap/brk |
| 配套 | 《C陷阱与缺陷》ch03、ch05 |

## 小节

- [11.1 为什么使用动态内存分配](./11.1-为什么使用动态内存分配.md)
- [11.2 malloc 和 free](./11.2-malloc和free.md)
- [11.3 calloc 和 realloc](./11.3-calloc和realloc.md)
- [11.4 使用动态分配的内存](./11.4-使用动态分配的内存.md)
- [11.5 常见错误](./11.5-常见错误.md)
- [11.6 内存分配实例](./11.6-内存分配实例.md)
