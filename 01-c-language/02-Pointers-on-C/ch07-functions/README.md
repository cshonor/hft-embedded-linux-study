# 第 7 章 函数

**Functions**

## 本章讲什么

**声明/定义、原型、值传递、return、栈调用、inline、可变参、递归、static 链接、ADT 黑盒、函数指针铺垫**。DPDK API、内核驱动、HFT 分层回调的函数模型全集。

## 学习重点

- **声明 vs 定义**；**`func(void)`** 非 `func()`
- **值传递**；大 struct 传**指针**；**const** 只读缓冲
- **禁止返回栈局部指针**
- **static** 函数 / **static inline** 头文件
- **va_list** 与 format 陷阱
- 递归 vs 迭代；HFT **禁深递归**
- 黑盒 ADT 接口

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | API 原型、inline、指针入参、rte_log |
| 内核 | static 私有、回调、栈 Oops |
| HFT | inline 降延迟、指针减拷贝 |

## 实操（建议完成）

1. 值传递 vs 指针传递  
2. static inline 位操作  
3. 简易 va_list 日志  
4. 返回栈数组错误  
5. 函数指针绑回调  
6. 递归 vs 循环链表  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02 链接；ch06 指针 |
| 后序 | ch11 malloc；ch13 函数指针；ch15 printf；ch18 ABI |
| 配套 | 《C陷阱与缺陷》ch04、ch05 |

## 小节

- [7.1 函数定义](./7.1-函数定义.md)
- [7.2 函数声明](./7.2-function-declarations/7.2-function-declarations.md)
  - [7.2.1 原型](./7.2-function-declarations/7.2.1-原型.md)
  - [7.2.2 函数的缺省认定](./7.2-function-declarations/7.2.2-函数的缺省认定.md)
- [7.3 函数的参数](./7.3-函数的参数.md)
- [7.4 ADT 和黑盒](./7.4-ADT和黑盒.md)
- [7.5 递归](./7.5-recursion/7.5-recursion.md)
  - [7.5.1 追踪递归函数](./7.5-recursion/7.5.1-追踪递归函数.md)
  - [7.5.2 递归与迭代](./7.5-recursion/7.5.2-递归与迭代.md)
- [7.6 可变参数列表](./7.6-variable-argument-lists/7.6-variable-argument-lists.md)
  - [7.6.1 stdarg 宏](./7.6-variable-argument-lists/7.6.1-stdarg宏.md)
  - [7.6.2 可变参数的限制](./7.6-variable-argument-lists/7.6.2-可变参数的限制.md)
