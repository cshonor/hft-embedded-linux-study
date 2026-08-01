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
