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


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 二级指针删头节点

```c
// 为什么删链表头节点要用二级指针？
void remove_head(Node **head) {
    Node *old = *head;
    if (old) {
        *head = old->next;
        free(old);
    }
}

// 如果用一级指针会怎样？
void remove_head_wrong(Node *head) {
    Node *old = head;
    head = head->next;  // 修改的是副本！
    free(old);
}
```

<details>
<summary>答案与复习指引</summary>

**答案：** `remove_head_wrong` 修改的是 `head` 的副本（传值），调用者的 `head` 指针不变 → 仍指向已释放的旧头节点 → **UAF**。

`remove_head` 用二级指针 `Node**`，通过 `*head = old->next` 直接修改调用者的指针变量。

**教训：** 要在函数内修改调用者的指针变量，必须传指针的地址（二级指针）。

**复习：** → [12.2 Singly Linked Lists](./12.2-singly-linked-lists/12.2-singly-linked-lists.md)

</details>


### Q3: 链表反转

```c
struct Node {
    int val;
    struct Node *next;
};

// 反转单链表：1→2→3→NULL 变成 3→2→1→NULL
struct Node *reverse(struct Node *head) {
    struct Node *prev = NULL, *curr = head, *next;
    while (curr) {
        next = curr->next;    // A: 保存下一个
        curr->next = prev;    // B: 反转指向
        prev = curr;          // C: 前进
        curr = next;          // D: 前进
    }
    return prev;
}
```

> 如果删掉 A 行（`next = curr->next`），会发生什么？为什么需要临时变量 `next`？

<details>
<summary>答案与复习指引</summary>

**答案：** 删掉 A 行后，B 行 `curr->next = prev` 把 `curr->next` 指向 `prev`——**丢失了原链表的后续节点**。D 行 `curr = next` 中 `next` 未赋值（或为旧值）——链表断裂，后续节点全部泄漏。

**为什么需要 `next`：** 反转指针方向后，原来的 `curr->next` 被覆盖，需要事先保存才能继续遍历。这是**指针操作的核心**——修改指针前先保存它指向的下一个目标。

**复习：** → [12.2 单链表](./12.2-singly-linked-lists/12.2-singly-linked-lists.md)

</details>

### Q4: 双链表删除

```c
struct DNode {
    int val;
    struct DNode *prev, *next;
};

void delete_node(struct DNode *node) {
    node->prev->next = node->next;    // A
    node->next->prev = node->prev;    // B
    free(node);
}

// 如果 node 是头节点（prev=NULL）或尾节点（next=NULL）会怎样？
```

> 上面的代码处理了头/尾节点的情况吗？如何修复？

<details>
<summary>答案与复习指引</summary>

**答案：** **没有处理**。如果 `node->prev == NULL`（头节点），A 行解引用 NULL → **崩溃**。如果 `node->next == NULL`（尾节点），B 行解引用 NULL → **崩溃**。

**修复：**
```c
void delete_node(struct DNode **head, struct DNode *node) {
    if (node->prev)
        node->prev->next = node->next;
    else
        *head = node->next;          // 更新头指针
    if (node->next)
        node->next->prev = node->prev;
    free(node);
}
```

**规则：** 操作指针前**必须判空**。双链表删除需要处理四种情况：中间节点、头节点、尾节点、唯一节点。

**复习：** → [12.3 双链表](./12.3-doubly-linked-lists/12.3-doubly-linked-lists.md)

</details>


### Q2: 嵌套堆成员分层释放

```c
struct Msg {
    int type;
    char *payload;   // 堆分配
};

struct Msg *m = malloc(sizeof(struct Msg));
m->payload = malloc(1000);

// 正确释放顺序？
```

<details>
<summary>答案与复习指引</summary>

**正确顺序：** 先 `free(m->payload)`（内层），再 `free(m)`（外层）。反过来 → 外层已释放，`m->payload` 变悬垂 → 读到垃圾值或崩溃。

```c
free(m->payload);
free(m);
```

**教训：** 嵌套堆分配按**逆序释放**（类似 `goto` 清理链）。

**复习：** → [12.1 链表](./12.1-链表.md)

---

## 代码自测

**题目 1：** 以下链表删除节点的代码有什么问题？
```c
void delete(Node *head, int val) {
    Node *cur = head;
    while (cur->next && cur->next->val != val)
        cur = cur->next;
    if (cur->next) {
        Node *tmp = cur->next;
        cur->next = tmp->next;
        // 没有 free(tmp)
    }
}
```

<details>
<summary>参考答案</summary>

内存泄漏——删除的节点 tmp 没有被 free。虽然从链表中移除了，但内存仍然占用。正确做法：free(tmp); 在 cur->next = tmp->next; 之后。此外，如果要删除的是头节点，这段代码无法处理（需要二级指针或返回新头）。

</details>
