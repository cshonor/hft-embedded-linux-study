## ① 链表 · Linked Lists

内核里 **最简单、最常用** 的结构。

#### 嵌入式设计（与教科书不同）

| 常见做法 | Linux 内核做法 |
|----------|----------------|
| 节点 = 你的结构体 | **`struct list_head` 嵌进** 你的结构体 |
| 一个 struct 即链表元素 | 一个 struct 可有 **多个** `list_head`（多链表） |

```c
/* 教科书写法：节点直接包数据 —— 每种业务结构体都要重写一套增删遍历代码 */
struct node {
    int data;
    struct node *next, *prev;
};

/* 内核做法：list_head 只是一小块指针，不含任何业务数据 */
struct list_head {
    struct list_head *next, *prev;
};

/* 业务结构体把 list_head 嵌进来 */
struct my_task {
    int data;
    char name[16];
    struct list_head list;   /* 嵌入链表节点 */
};
```

> 链表串起来的是一个个 `list_head`，**不是 `my_task` 本身**——只拿到 `list_head *`，必须靠 `container_of` 反向找回宿主。

#### `container_of()` — 从节点找回父结构

依赖 **GCC 扩展**（`typeof` 等）— 见 [Ch2 §2.4 GNU C](../../chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)。纯 ISO C 很难优雅实现同等宏。

```c
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) ); })

/* 从 list 成员指针 → 外层 my_task * */
struct my_task *p = list_entry(ptr, struct my_task, list);
/* list_entry 基于 container_of */
```

**三步拆解**（`ptr`=手上的 `list_head *`，`type`=宿主类型，`member`=宿主里那个成员名）：

1. `offsetof(type, member)`：算出 `list` 成员相对结构体开头的**字节偏移**（`(size_t)&((type *)0)->member` 的思路——把 NULL 当结构体首地址，成员地址即偏移量）；
2. `(char *)__mptr - offsetof(...)`：成员地址**减去偏移** = 宿主结构体起始地址；
3. 强转回 `type *`。`typeof` 一步只做**编译期类型检查**（保证 ptr 真是 member 的类型），标准 C 没有它——所以这套只能在内核/GNU 环境用。

**一个宿主可挂多条链表**（教科书做不到的杀手锏）：

```c
struct my_task {
    int data;
    struct list_head run_list;    /* 挂运行队列 */
    struct list_head wait_list;   /* 同时挂等待队列 */
};
/* 同一对象同时在两条无关链表上 —— 内核大量使用 */
```

**遍历**（宏内部就是 container_of）：

```c
struct my_task *pos;                 /* 宿主类型指针，不是 list_head* */
list_for_each_entry(pos, &head, list) {
    /* pos 已是 struct my_task*，直接访问 pos->data */
}
```

| 宏/函数 | 复杂度 | 作用 |
|---------|--------|------|
| `list_add()` / `list_add_tail()` | **O(1)** | 插入 |
| `list_del()` | **O(1)** | 删除 |
| `list_for_each_entry()` | O(n) 遍历 | **类型安全** 遍历 |

```
task list（Ch 3）概念：
  task_struct ──list_head──► task_struct ──list_head──► …
```

→ **Ch 3** 任务队列 · 等待队列（Ch 4/9）亦常用链表



<details>
<summary>自测题（点击展开）</summary>

**Q1.** list_head 和教科书链表有什么区别？container_of 宏怎么工作？

<details><summary>答案</summary>

教科书链表：node { void *data; node *next; }，数据在外部分配。内核 list_head：嵌入在数据结构内，通过 `container_of(ptr, type, member)` 计算宿主地址 = (char*)ptr - offsetof(type, member)。这样不需要额外分配节点，一个对象可同时挂在多个链表上。

</details>

**Q2.** list_for_each_safe 和 list_for_each 的区别？什么时候用 safe 版本？

<details><summary>答案</summary>

list_for_each 在遍历中删除当前节点会导致 use-after-free。list_for_each_safe 额外保存 next 指针，可以在遍历中安全删除当前节点。内核删除链表节点的正确模式：`list_for_each_safe(pos, n, head) { list_del(pos); kfree(...); }`。

</details>

</details>
---
