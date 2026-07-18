# 第 3 章 数据

**Data**

## 本章讲什么

**底层数据存储与二进制内存模型**：整型/浮点/指针类型、**常量**与 **enum**、**const/volatile/restrict**、**typedef** 定长、**作用域/链接/存储期** 与 **.text/.data/.bss/.rodata/栈/堆**。看懂 DPDK mbuf、内核 struct、交易报文、寄存器读写的地基。

## 学习重点

- 定宽整型、**整数提升**、**unsigned 混合比较**
- HFT：**定点价格** vs float；**htonl** 大小端
- **const** 四式指针、**volatile** MMIO、**restrict** 优化
- **.data/.bss/.rodata** 与初始化/脏栈
- **static** 双义、**extern** 链接
- **enum**、字符串 **.rodata** 只读

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| 驱动/寄存器 | volatile + uint8/16 + 十六进制常量 |
| 报文解析 | 无符号字节、endian、隐式转换 |
| DPDK | typedef、const 缓冲、restrict、enum 类型 |
| 低延迟 | 段划分、减少冗余访存 |

## 实操（建议完成）

1. `sizeof` 各类型（32/64 位）  
2. 改字符串常量 → 段错误  
3. `unsigned char` vs `-1` 比较  
4. volatile 对比汇编（`gcc -S -O2`）  
5. enum + typedef  
6. `readelf -S`  
7. 自实现 **htonl**  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02 作用域、static/extern |
| 后序 | ch05 运算；ch06 指针；ch10 对齐；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [3.1 基本数据类型](./3.1-basic-data-types/3.1-basic-data-types.md)
  - [3.1.1 整型家族](./3.1-basic-data-types/3.1.1-整型家族.md)
  - [3.1.2 浮点类型](./3.1-basic-data-types/3.1.2-浮点类型.md)
  - [3.1.3 指针](./3.1-basic-data-types/3.1.3-指针.md)
- [3.2 基本声明](./3.2-basic-declarations/3.2-basic-declarations.md)
  - [3.2.1 初始化](./3.2-basic-declarations/3.2.1-初始化.md)
  - [3.2.2 声明简单数组](./3.2-basic-declarations/3.2.2-声明简单数组.md)
  - [3.2.3 声明指针](./3.2-basic-declarations/3.2.3-声明指针.md)
  - [3.2.4 隐式声明](./3.2-basic-declarations/3.2.4-隐式声明.md)
- [3.3 typedef](./3.3-typedef.md)
- [3.4 常量](./3.4-常量.md)
- [3.5 作用域](./3.5-scope/3.5-scope.md)
  - [3.5.1 代码块](./3.5-scope/3.5.1-代码块.md)
  - [3.5.2 文件](./3.5-scope/3.5.2-文件.md)
  - [3.5.3 原型](./3.5-scope/3.5.3-原型.md)
  - [3.5.4 函数](./3.5-scope/3.5.4-函数.md)
- [3.6 链接属性](./3.6-链接属性.md)
- [3.7 存储类型](./3.7-存储类型.md)
- [3.8 static 关键字](./3.8-static关键字.md)
- [3.9 作用域、存储类型示例](./3.9-作用域-存储类型示例.md)
