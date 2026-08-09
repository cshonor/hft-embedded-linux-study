# 第 9 章 字符串、字符和字节

**Strings, Characters, and Bytes**

## 本章讲什么

**C 字符串（`\0`）** vs **原始二进制字节流（显式 len）**；**str\*** 与 **mem\*** 分工；**ctype**；指针遍历 payload。报文解析、寄存器读写、DPDK mbuf 的核心章节。

## 学习重点

- **strlen ≠ sizeof**；无 `\0` 禁止 str*
- **char[]** 可写 vs **`const char *`** 字面量
- 文本：**str\*** / snprintf；二进制：**memcpy/memmove/memset/memcmp**
- **strncpy** 截断缺 `\0`；**memcpy** 重叠用 **memmove**
- **ctype**：`(unsigned char)c`
- 遍历：**`p < end`** 而非 strlen

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | payload + pkt_len + mem* |
| 内核 | 寄存器字节、memmove |
| HFT | memcmp 头校验、指针遍历 |

## 线上陷阱（汇总）

1. 二进制误用 strlen  
2. strncpy 无 `\0`  
3. 字面量无 const  
4. memcpy 重叠  
5. ctype 负 char  
6. 栈缓冲未清零  

## 实操（建议完成）

见 **9.9** 及章内各节。

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 指针；ch08 char 数组；ch05 位运算 |
| 后序 | ch10 协议 struct；ch15/ch16 I/O |
| 配套 | 《C陷阱与缺陷》ch03、ch05 |

## 小节

- [9.1 字符串基础](./9.1-字符串基础.md)
- [9.2 字符串长度](./9.2-字符串长度.md)
- [9.3 不受限制的字符串函数](./9.3-unrestricted-string-functions/9.3-unrestricted-string-functions.md)
- [9.4 长度受限的字符串函数](./9.4-长度受限的字符串函数.md)
- [9.5 字符串查找](./9.5-字符串查找.md)
- [9.6 高级字符串查找](./9.6-高级字符串查找.md)
- [9.7 错误信息](./9.7-错误信息.md)
- [9.8 字符操作](./9.8-字符操作.md)
- [9.9 内存操作](./9.9-内存操作.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: strlen vs sizeof

```c
char s[] = "hello";
printf("%zu\n", sizeof(s));   // (1)
printf("%zu\n", strlen(s));   // (2)

char *p = "hello";
printf("%zu\n", sizeof(p));   // (3)
printf("%zu\n", strlen(p));   // (4)
```

> 四个各输出多少？

<details>
<summary>答案与复习指引</summary>

**输出：** `(1)` = 6，`(2)` = 5，`(3)` = 8，`(4)` = 5

**解析：**
- `sizeof(s)` — 整个数组含 `\0` = 6
- `strlen(s)` — 不含 `\0` = 5
- `sizeof(p)` — 指针大小 = 8（64 位），与字符串无关
- `strlen(p)` — 运行时数到 `\0` = 5

**教训：** `sizeof` 是编译期计算，`strlen` 是运行时计算。

**复习：** → [9.1 String Length](./9.1-字符串长度.md)

</details>

### Q2: memcpy vs memmove

```c
char buf[10] = "abcdefghi";

memcpy(buf + 2, buf, 4);     // (1) 安全吗？
// memmove(buf + 2, buf, 4); // (2) 安全吗？
```

> 两个有什么区别？哪个安全？

<details>
<summary>答案与复习指引</summary>

**答案：** `(1)` **不安全**（源和目标重叠时 UB），`(2)` **安全**（`memmove` 正确处理重叠区域）。

`memcpy` 假设源和目标不重叠，可能用 SIMD 批量复制，重叠时数据损坏。`memmove` 先判断方向再复制，保证正确。

**教训：** 源和目标可能重叠时用 `memmove`，确定不重叠时用 `memcpy`（更快）。

**复习：** → [9.9 内存操作](./9.9-内存操作.md)

</details>

### Q3: strncpy 截断缺 \0

```c
char dst[4];
strncpy(dst, "hello world", sizeof(dst));
printf("%s\n", dst);  // 安全吗？
```

<details>
<summary>答案与复习指引</summary>

**答案：** **不安全**——`strncpy` 在缓冲区不够时**不补 `\0`**。`dst` 没有终止符，`printf("%s")` 会越界读直到遇到 `\0`。

**正确做法：** 手动补终止符：
```c
strncpy(dst, src, sizeof(dst) - 1);
dst[sizeof(dst) - 1] = '\0';
```
或用 `snprintf(dst, sizeof(dst), "%s", src)`。

**复习：** → [9.3 Unbounded String Functions](./9.3-字符串拷贝.md) — strncpy 陷阱

</details>

### Q4: ctype 与 unsigned char

```c
unsigned char c = 0xFF;  // 非 ASCII
if (isalpha(c))          // 安全吗？
    printf("alpha\n");
```

<details>
<summary>答案与复习指引</summary>

**答案：** **UB**——`isalpha` 的参数要求是 `EOF`（-1）或 `unsigned char` 范围的值。如果 `char` 是有符号的，`0xFF` 会被提升为 `int` = -1，而 -1 恰好等于 `EOF`。

**正确做法：** 强制转型：`isalpha((unsigned char)c)`。

**复习：** → [9.8 字符操作](./9.8-字符操作.md)
