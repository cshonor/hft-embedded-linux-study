# Ch13 · Storage（存储）

> **Level 2 · 相知** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

> **第 4 本书 · Ch13** · HFT 热路径不用 `malloc`——启动时一次性分配 + 自管理内存池，理解四种存储期是设计内存池的基础。

## 本章讲什么

C 的四种存储期（storage duration）、`malloc` 家族与 `realloc` 陷阱、初始化规则全表、
机器模型抽象。`_Thread_local` 是 DPDK 每 lcore 独立数据的语言基础。

## 小节索引

| 节 | 标题 | 核心知识点 |
|----|------|------------|
| [13.1](./13.1-malloc和友员.md) | malloc 和友员 | malloc/calloc/realloc/aligned_alloc；realloc 陷阱 |
| [13.2](./13.2-存储持续时间.md) | 存储持续时间 | **核心**：自动/静态/线程/分配四种存储期；`_Thread_local` |
| [13.3](./13.3-在定义对象之前使用对象.md) | 在定义对象之前使用对象 | 不完整类型；opaque struct 封装 |
| [13.4](./13.4-初始化.md) | 初始化 | 初始化规则全表；C23 `{}` 零初始化 |
| [13.5](./13.5-机器模型.md) | 机器模型 | 段布局 .text/.data/.bss/stack/heap；`register` 废弃 |
| [13.6](./13.6-HFT内存池模型.md) | HFT 内存池模型（补充） | 为什么不用 malloc；内存池设计；DPDK rte_mempool |

## HFT / DPDK 关联总结

| 概念 | HFT 应用 |
|------|----------|
| **四种存储期** | 设计内存池、理解 `_Thread_local` 模型 |
| **`_Thread_local`** | 每 lcore 独立数据（统计计数器、本地缓存） |
| **内存池** | 启动时预分配，运行时 O(1) 分配/释放 |
| **`calloc`** | 零初始化分配（比 malloc 安全） |
| **`aligned_alloc`** | DMA 缓冲区对齐分配 |
| **`realloc` 陷阱** | 用临时变量接收返回值 |
| **初始化规则** | 局部变量必须显式初始化 |
| **不用 `malloc` 在热路径** | 延迟不可控 |

## 自测题

<details><summary>1. 四种存储期分别是什么？各自何时创建/销毁？</summary>

① 自动存储期（局部变量）：进入块时创建，退出块时销毁。② 静态存储期（全局/static）：
程序开始时创建，程序结束时销毁。③ 线程存储期（`_Thread_local`）：线程开始时创建，
线程退出时销毁。④ 分配存储期（malloc）：malloc 时创建，free 时销毁。
</details>

<details><summary>2. 为什么 HFT 热路径不用 <code>malloc</code>？怎么替代？</summary>

malloc 的延迟不可预测：可能触发 mmap/brk 系统调用、需要锁竞争、可能 page fault。
HFT 要求微秒级确定性延迟。替代方案：启动时一次性分配大块内存（hugepage），运行时从内存池
O(1) 切分（空闲链表或 slab 分配器），无锁（每 lcore 独立 cache）。DPDK `rte_mempool` 就是工业实现。
</details>

<details><summary>3. <code>arr = realloc(arr, new_size)</code> 有什么问题？正确写法是什么？</summary>

realloc 失败时返回 NULL，但原内存仍有效。`arr = realloc(arr, new_size)` 在失败时把 NULL 赋给 arr，
丢失了原指针 → 内存泄漏。正确写法：`int *tmp = realloc(arr, new_size); if (!tmp) { free(arr); return -1; } arr = tmp;`
</details>

<details><summary>4. <code>_Thread_local</code> 和全局变量有什么区别？</summary>

全局变量是所有线程共享的——一个线程修改，其它线程立刻看到（需同步）。`_Thread_local` 变量是
每个线程独立一份——一个线程修改不影响其它线程的副本，不需要同步。HFT 中每 lcore 的统计计数器
用 `_Thread_local`，避免共享缓存行的伪共享和锁开销。
</details>

<details><summary>5. 为什么局部变量不初始化是 HFT bug 的常见原因？</summary>

自动存储期的局部变量不自动清零——初值是栈上的垃圾值。debug 构建中栈可能碰巧是零，
release 构建中优化后栈内容不同，导致"debug 正常、release 偶发"的 bug。HFT 代码规范应要求
所有局部变量声明时初始化：`int x = 0;` 或 `int *p = NULL;`。
</details>
