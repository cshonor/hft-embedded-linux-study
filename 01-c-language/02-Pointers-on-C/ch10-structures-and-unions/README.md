# 第 10 章 结构和联合

**Structures and Unions**

## 本章讲什么

用 **struct / union / bit-field** 建模二进制数据：协议头、硬件寄存器、DPDK 数据结构。核心难题：**对齐 padding**、**packed**、大小端、传指针 vs 拷贝。

## 学习重点

- **`.` / `->`**；嵌套 struct；**自引用指针**链表
- **对齐与 padding**；线格式用 **packed** + **memcpy** + 字节序转换
- **union** 共享内存、变体记录 + **type 判别**
- **位域**仅本地寄存器，**禁止**网络协议
- 大 struct **传指针**，避免 HFT 热路径拷贝
- **C11** 匿名 struct/union 简化协议头

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | `rte_mbuf` 嵌套 struct；packed 协议头 |
| 内核 | bit-field 寄存器；`task_struct` 嵌套 |
| HFT | packed 行情/订单头；union 字节拆分；链表节点 |

## 线上陷阱（汇总）

1. 默认 padding 导致协议字段偏移  
2. packed 非对齐 → ARM 崩溃 / x86 变慢  
3. union 无判别混读  
4. 位域用于线格式  
5. 大 struct 按值传递  
6. **`->`** 空指针  
7. 自引用写成实体成员 `struct node next`  

## 实操（建议完成）

1. `sizeof` 验证 padding  
2. packed 前后大小对比  
3. union 拆分 32 位 + 大小端  
4. bit-field 寄存器模拟  
5. 自引用链表插入/遍历  
6. packed struct + **memcpy** 载入报文  
7. 按值 vs 指针传递对比  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch03 定长类型；ch06 指针；ch09 mem* 字节流 |
| 后序 | ch11 堆 struct；ch12 指针链表；ch15 fread/fwrite |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [10.1 结构基础知识](10.1-structure-basics/10.1-结构基础知识.md)
- [10.2 结构的访问](10.2-accessing-structures/10.2-结构的访问.md)
- [10.3 结构的存储分配](./10.3-结构的存储分配.md)
- [10.4 作为函数参数的结构](./10.4-作为函数参数的结构.md)
- [10.5 位段](./10.5-位段.md)
- [10.6 联合](10.6-unions/10.6-联合.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: packed 结构体

```c
struct __attribute__((packed)) Header {
    uint8_t  type;     // 1
    uint32_t seq;      // 4
    uint16_t len;      // 2
};

printf("%zu\n", sizeof(struct Header));  // 多少？
// 不加 packed 呢？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 加 `packed` → `sizeof = 7`。不加 `packed` → `sizeof = 12`（对齐填充）。

`packed` 移除所有 padding，成员紧密排列。协议头、报文结构必须 `packed`，否则网络上 / 内存映射的布局与预期不一致。

**代价：** `packed` 结构体中 `uint32_t` 可能未对齐 → 某些架构（ARM）上访问未对齐地址 → **总线错误** 或性能下降。x86 允许未对齐访问但有性能损失。

**复习：** → [10.1 Structure Declaration](10.1-structure-basics)

</details>

### Q2: union 变体记录

```c
struct Message {
    int type;
    union {
        struct { int x, y; }   coords;
        struct { char name[8]; } info;
    } payload;
};

struct Message m;
m.type = 1;
m.payload.coords.x = 10;

// 安全访问 m.payload.info.name 吗？
```

<details>
<summary>答案与复习指引</summary>

**答案：** **不安全**——union 各成员共享内存，当前活跃的是 `coords`，读 `info.name` 得到的是 `coords` 的字节重新解释。安全做法是先判 `type` 再访问对应 union 成员。

**教训：** union + type 字段 = C 语言的"变体类型"，但编译器不帮你检查，全靠程序员自律。

**复习：** → [10.6 Unions](10.6-unions/10.6-联合.md)

</details>


### Q3: 位域布局与可移植性

```c
struct Flags {
    unsigned a : 3;   // 3 bits
    unsigned b : 4;   // 4 bits
    unsigned c : 1;   // 1 bit
};

struct Flags f = {5, 10, 1};
printf("%zu\n", sizeof(f));   // 输出多少？

// 跨平台安全吗？
f.a = 8;   // 3 bits 最大值是 7
```

> `sizeof(f)` 是多少？`f.a = 8` 会发生什么？位域布局跨平台一致吗？

<details>
<summary>答案与复习指引</summary>

**答案：** `sizeof(f)` = **4**（一个 `unsigned int`，3+4+1=8 bits 恰好填满，但实现可能 padding 到 `unsigned int` 宽度）。

`f.a = 8`：3 bits 最大存 7（`0b111`），`8 = 0b1000` 超出范围——值被截断为 `0`（实现定义行为）。

**跨平台问题：** 位域的内存排列顺序（MSB vs LSB）、是否跨字节边界、`signed`/`unsigned` 默认——全部是实现定义。**不同编译器/架构布局不同**。

**规则：** 协议头和硬件寄存器**不用位域**——用移位和掩码手动操作。位域只用于同一平台内的紧凑存储。

**复习：** → [10.5 位段](./10.5-位段.md) · [10.1 结构声明](10.1-structure-basics)

</details>

### Q4: 结构体赋值是浅拷贝

```c
struct Person {
    char *name;
    int age;
};

struct Person p1 = {strdup("Alice"), 30};
struct Person p2 = p1;     // 直接赋值

free(p1.name);
printf("%s\n", p2.name);   // 安全吗？
```

> `p2.name` 还能安全访问吗？如何实现深拷贝？

<details>
<summary>答案与复习指引</summary>

**答案：** **不安全**——结构体直接赋值是**浅拷贝**：`p2.name` 和 `p1.name` 指向同一块 `strdup` 分配的内存。`free(p1.name)` 后 `p2.name` 变成**悬垂指针**（use-after-free）。

**深拷贝：**
```c
p2.age = p1.age;
p2.name = strdup(p1.name);  // 独立分配
```

**规则：** 含指针成员的结构体不能靠赋值拷贝——必须写专门的拷贝函数。C 没有拷贝构造函数。

**复习：** → [10.4 作为函数参数的结构](./10.4-作为函数参数的结构.md)

</details>

---

## 代码自测

**题目 1：** 以下结构体在 64 位系统上 sizeof 是多少？
```c
struct S {
    char a;
    int b;
    char c;
};
```

<details>
<summary>参考答案</summary>

通常是 12 字节。由于对齐要求，a 占 1 字节后填充 3 字节到 b（偏移 4），b 占 4 字节，c 占 1 字节后填充 3 字节使总大小是 4 的倍数。布局：a(1) + pad(3) + b(4) + c(1) + pad(3) = 12。如果改为 struct S { int b; char a; char c; }; 则 sizeof = 8。

</details>
