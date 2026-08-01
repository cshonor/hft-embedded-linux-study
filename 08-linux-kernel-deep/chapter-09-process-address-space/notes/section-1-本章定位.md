## 1. 本章定位

> **ULK Ch 9 Process Address Space** · 内核如何为用户进程管理 **虚拟内存**

---

### 一、本章讲什么

Ch 8 讲 **物理页框** 怎么分配；本章讲 **用户进程** 如何获得和使用内存：

| 策略 | 含义 |
|------|------|
| **延迟分配** | 先给 **线性地址区间**（VMA），物理页框 **访问时才分配** |
| **缺页驱动** | 真正访问 → 缺页异常 → `do_page_fault()` 分配/映射 |

核心对象：**`mm_struct`**（地址空间）、**`vm_area_struct`**（内存区）、**缺页处理路径**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-内存描述符.md) | `mm_struct`、`mm_users`/`mm_count`、内核线程 |
| [3](./section-3-内存区VMA.md) | `vm_area_struct`、红黑树 + 链表、`do_mmap`/`do_munmap` |
| [4](./section-4-缺页异常.md) | `do_page_fault()`、SIGSEGV vs 合法缺页 |
| [5](./section-5-请求调页.md) | 匿名页、零页、读/写路径 |
| [6](./section-6-写时复制与堆.md) | COW/`do_wp_page()`、`brk()` 堆管理 |

---

### 三、在 Linux 链上的位置

```
Ch 2  页表 / 线性地址
Ch 3  fork（COW 预告）
Ch 8  物理页分配（伙伴 / Slab）
Ch 9  进程虚拟地址空间（本章）
Ch 10 brk/mmap 系统调用入口
Ch 17 页回收 / swap
```

HFT：**mmap 大页、预 touch、避免缺页抖动** 都建立在本章机制之上。

---

← [Ch 9 导读](../README.md) · 下一节 [2. 内存描述符](./section-2-内存描述符.md)
