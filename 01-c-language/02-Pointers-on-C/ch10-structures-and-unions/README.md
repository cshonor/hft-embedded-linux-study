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

- [10.1 结构基础知识](./10.1-structure-basics/10.1-structure-basics.md)
- [10.2 结构的访问](./10.2-accessing-structures/10.2-accessing-structures.md)
- [10.3 结构的存储分配](./10.3-结构的存储分配.md)
- [10.4 作为函数参数的结构](./10.4-作为函数参数的结构.md)
- [10.5 位段](./10.5-位段.md)
- [10.6 联合](./10.6-unions/10.6-unions.md)


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

**复习：** → [10.1 Structure Declaration](./10.1-结构声明.md)

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

**复习：** → [10.6 Unions](./10.6-unions/10.6-unions.md)

</details>
