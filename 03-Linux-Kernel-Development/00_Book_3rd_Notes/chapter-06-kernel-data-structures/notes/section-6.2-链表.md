## ① 链表 · Linked Lists

内核里 **最简单、最常用** 的结构。

#### 嵌入式设计（与教科书不同）

| 常见做法 | Linux 内核做法 |
|----------|----------------|
| 节点 = 你的结构体 | **`struct list_head` 嵌进** 你的结构体 |
| 一个 struct 即链表元素 | 一个 struct 可有 **多个** `list_head`（多链表） |

```c
struct my_task {
    int data;
    struct list_head list;   /* 嵌入的链表节点 */
};
```

#### `container_of()` — 从节点找回父结构

```c
/* 从 list 成员指针 → 外层 my_task * */
struct my_task *p = list_entry(ptr, struct my_task, list);
/* list_entry 基于 container_of */
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

---
