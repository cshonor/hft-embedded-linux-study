# 第 6 章 结构

**Structures, Unions, Bit-fields**

## 本章讲什么

**结构体、共用体、位域、typedef、自引用链表**。硬件寄存器、报文解析、内存布局、UEFI 协议、HFT 行情包、飞控传感器存储都建立在本章。承接第 5 章指针，为二进制数据处理铺路。

## 学习重点

- **内存对齐与 padding** —— 本章最大难点；ELF / 协议 / `sizeof` 面试必考
- **`.` vs `->`**、传 struct 值 vs **传指针**
- **struct vs union**、位域**不可移植**
- **自引用链表**、6.6 符号表综合案例
- **typedef** 与定宽类型（UEFI / 内核风格）

## 场景映射

| 方向 | 本章技能 |
|------|----------|
| OS / UEFI | MemoryMap、Protocol 嵌套 struct、EFI 类型别名 |
| HFT | 订单/行情 struct、指针传参减拷贝、定长 typedef |
| 嵌入式 / 飞控 | 寄存器 union/位域、设备链表、PID 参数 struct |

## 难度

中等偏上；业务层 C 用得少，**底层 / 硬件 / 网络二进制**绕不开。

## 小节

- [**6.0 结构体 · 驱动向速通**](./6.0-struct-for-drivers.md)（写驱动前先过这一页）
- [6.1 结构的基本知识](./6.1-结构的基本知识.md)
- [6.2 结构与函数](./6.2-结构与函数.md)
- [6.3 结构数组](./6.3-结构数组.md)
- [6.4 指向结构的指针](./6.4-指向结构的指针.md)
- [6.5 自引用结构](./6.5-自引用结构.md)
- [6.6 表查找](./6.6-表查找.md)
- [6.7 类型定义 typedef](./6.7-类型定义typedef.md)
- [6.8 联合](./6.8-联合.md)
- [6.9 位字段](./6.9-位字段.md)

---

## 章节自测

> 结构体是二进制数据处理的核心。硬件寄存器、报文解析、内核协议都靠它。看代码 → 想答案 → 点开验证。

### Q1: sizeof 与 padding

```c
#include <stdio.h>

struct A {
    char c;    // 1 byte
    int  i;    // 4 bytes
    char d;    // 1 byte
};

struct B {
    int  i;    // 4 bytes
    char c;    // 1 byte
    char d;    // 1 byte
};

printf("A = %zu\n", sizeof(struct A));
printf("B = %zu\n", sizeof(struct B));
```

> `sizeof(struct A)` 和 `sizeof(struct B)` 各是多少？为什么不同？

<details>
<summary>答案与复习指引</summary>

**输出：** `A = 12`，`B = 8`

**解析：**
- `struct A`：`c`(1) + `padding`(3) + `i`(4) + `d`(1) + `padding`(3) = 12
- `struct B`：`i`(4) + `c`(1) + `d`(1) + `padding`(2) = 8

**内存对齐规则：** 每个成员对齐到自身大小的整数倍地址。`struct A` 中 `int i` 前面有 1 字节 `char`，需填充 3 字节才能 4 对齐。尾部也要补齐到最大成员（`int` = 4）的整数倍。

**教训：** 结构体成员顺序影响 sizeof。从大到小排列可减小 padding。

**复习：** → [6.1 结构的基本知识](./6.1-结构的基本知识.md) — 对齐与 padding

</details>

### Q2: 点 vs 箭头

```c
struct Point { int x, y; };

struct Point p = {3, 4};
struct Point *q = &p;

printf("%d\n", p.x);    // (1)
printf("%d\n", (*q).y); // (2)
printf("%d\n", q->x);   // (3)

q->y = 99;
printf("%d\n", p.y);    // (4)
```

> 四行各输出多少？`.` 和 `->` 有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `(1)` = 3，`(2)` = 4，`(3)` = 3，`(4)` = 99

**解析：**
- `.` 用于直接访问结构体（变量名）
- `->` 用于通过指针访问（指针解引用 + 取成员的语法糖）
- `(*q).y` ≡ `q->y`（完全等价，箭头是语法糖）
- `q` 指向 `p`，修改 `q->y` = 修改 `p.y`

**内核/驱动惯例：** 结构体几乎总是传指针（避免拷贝 + 可修改），所以 `->` 用得比 `.` 多。

**复习：** → [6.4 指向结构的指针](./6.4-指向结构的指针.md)

</details>

### Q3: struct vs union

```c
union Data {
    int   i;
    float f;
    char  bytes[4];
};

union Data d;
d.i = 0x41424344;

printf("%c%c%c%c\n", d.bytes[0], d.bytes[1], d.bytes[2], d.bytes[3]);
d.f = 1.0f;
printf("%d\n", d.i);  // 还是 0x41424344 吗？
```

> union 的 `sizeof` 是多少？第二次 `printf` 输出什么？

<details>
<summary>答案与复习指引</summary>

**答案：** `sizeof(union Data) = 4`（最大成员大小）。第二次 `printf` 输出 `d.i` 的值 = `1.0f` 的位模式重新解释为 `int`（典型值 `0x3F800000` = 1065353216）。

**解析：** union 所有成员**共享同一段内存**。写 `d.i` 后 `d.bytes` 读的是同一段数据的字符视图。写 `d.f` 后 `d.i` 读的是 float 的位模式重新当 int——原始值被覆盖了。

**用途：** 硬件寄存器（同一地址既是控制位又是状态位）、协议解析（同一缓冲区不同头类型解释）。

**复习：** → [6.8 联合](./6.8-联合.md)

</details>

### Q4: 自引用结构与链表

```c
struct Node {
    int data;
    struct Node *next;  // 为什么是指针不能是 struct Node？
};

struct Node a = {1, NULL};
struct Node b = {2, &a};
struct Node *head = &b;

printf("%d\n", head->data);       // (1)
printf("%d\n", head->next->data); // (2)
```

> 为什么 `next` 必须是指针？如果写成 `struct Node next;` 会怎样？`(1)` `(2)` 各输出多少？

<details>
<summary>答案与复习指引</summary>

**输出：** `(1)` = 2，`(2)` = 1

**解析：** `struct Node` 内部不能包含 `struct Node`（无限大小，编译器无法确定 sizeof）。但可以包含 `struct Node *`（指针大小固定，不依赖 `struct Node` 完整定义）。这是自引用结构的核心。

`head` → `b{data=2, next=&a}` → `a{data=1, next=NULL}`

**内核大量使用：** `list_head`、`task_struct`、`page` 等。

**复习：** → [6.5 自引用结构](./6.5-自引用结构.md)

</details>

### Q5: typedef

```c
typedef struct {
    int x, y;
} Point;

typedef void (*handler_t)(int);

Point p = {10, 20};
handler_t h = NULL;  // 函数指针别名
```

> `typedef` 做什么？为什么内核大量使用 typedef？

<details>
<summary>答案与复习指引</summary>

**答案：** `typedef` 给类型起别名。`Point` = 匿名 struct 的别名，`handler_t` = `void(*)(int)` 函数指针的别名。

**解析：** typedef 不创建新类型，只是已有类型的别名。内核大量使用 typedef：
- 定宽类型：`u32`、`pid_t`、`size_t`
- 函数指针：`irq_handler_t`、`file_op` 操作表
- 不透明类型：`struct file` 的 typedef 隐藏内部实现

**注意：** POSIX 中 `typedef struct Node *NodePtr;` 比 `struct Node *` 更简洁。

**复习：** → [6.7 类型定义typedef](./6.7-类型定义typedef.md)

</details>

### Q6: 位域

```c
struct Flags {
    unsigned int a : 1;  // 1 bit
    unsigned int b : 3;  // 3 bits
    unsigned int c : 4;  // 4 bits
};

struct Flags f;
f.a = 1;
f.b = 5;   // 5 = 0b101，放 3 bit 里 OK
f.c = 15;  // 15 = 0b1111，放 4 bit 刚好

printf("%u %u %u\n", f.a, f.b, f.c);
f.c = 20;  // 20 = 0b10100，超出 4 bit
printf("%u\n", f.c);  // 输出什么？
```

> 位域的 `:` 后面数字是什么意思？`f.c = 20` 后输出多少？

<details>
<summary>答案与复习指引</summary>

**输出：** 第一行 `1 5 15`，第二行 `4`

**解析：** `unsigned c : 4` 表示 `c` 只占 4 个 bit。`20 = 0b10100` 截断为低 4 位 = `0b0100` = 4。

**位域用途：** 紧凑存储标志位（协议头、硬件寄存器映射）。

**致命陷阱：** 位域的**内存排列顺序（MSB/LSB first）、对齐方式、是否跨字节**全部是**实现定义的**。不同编译器、不同架构结果可能不同。跨平台不可移植，同一平台可用。

**教训：** 协议解析别用位域，用位操作（`shift + mask`）才可移植。

**复习：** → [6.9 位字段](./6.9-位字段.md)
