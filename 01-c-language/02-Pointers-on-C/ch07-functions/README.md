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
  - [7.3.1 值拷贝无例外](./7.3.1-值拷贝无例外.md) ← int/指针/struct/数组 · 思考题
- [7.4 ADT 和黑盒](./7.4-ADT和黑盒.md)
- [7.5 递归](./7.5-recursion/7.5-recursion.md)
  - [7.5.1 追踪递归函数](./7.5-recursion/7.5.1-追踪递归函数.md)
  - [7.5.2 递归与迭代](./7.5-recursion/7.5.2-递归与迭代.md)
- [7.6 可变参数列表](./7.6-variable-argument-lists/7.6-variable-argument-lists.md)
  - [7.6.1 stdarg 宏](./7.6-variable-argument-lists/7.6.1-stdarg宏.md)
  - [7.6.2 可变参数的限制](./7.6-variable-argument-lists/7.6.2-可变参数的限制.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: func(void) vs func()

```c
// 声明 A
int foo(void);

// 声明 B
int bar();
```

> `foo(void)` 和 `bar()` 有什么区别？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `foo(void)` — 明确**不接受参数**，传参会报错
- `bar()` — C89 中表示**接受任意数量参数**（不检查），C++ 中等同 `void`

**教训：** 无参函数写 `void`，别留空括号。

**复习：** → [7.1 Function Definition](./7.1-function-definition/7.1-function-definition.md)

</details>

### Q2: 禁止返回栈指针

```c
int *bad_func(void) {
    int local = 42;
    return &local;  // 合法吗？
}

int main(void) {
    int *p = bad_func();
    printf("%d\n", *p);  // 会怎样？
    return 0;
}
```

<details>
<summary>答案与复习指引</summary>

**答案：** **UB**——返回栈局部变量的地址。函数返回后栈帧被回收，`p` 成为悬垂指针。

**可能结果：** 程序看似正常（栈还没被覆盖），也可能输出垃圾值或崩溃。

**正确做法：** 用 `malloc` 分配堆内存返回，或通过参数指针输出，或用 `static` 变量。

**复习：** → [7.3 Function Arguments](./7.3-function-arguments/7.3-function-arguments.md)

</details>

### Q3: static inline 头文件

```c
// math_utils.h
static inline int max(int a, int b) {
    return a > b ? a : b;
}

// 被 10 个 .c 文件 #include
// 会产生 multiple definition 吗？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 不会。`static` 使每个编译单元有自己的副本（内部链接），不产生全局符号冲突。`inline` 建议编译器内联展开。

**权衡：** 多份副本增加代码体积，但 `inline` 展开后没有函数调用开销。内核大量使用 `static inline` 在头文件中定义小函数。

**复习：** → [7.4 Recursion](./7.4-recursion/7.4-recursion.md) — static inline

</details>

### Q4: va_list 陷阱

```c
int sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);
    va_end(ap);
    return total;
}

// 调用
sum(3, 1, 2, 3);      // (1) OK
sum(3, 1, 2);         // (2) 会怎样？
sum(3, 1, 2.0, 3);    // (3) 会怎样？
```

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` = 6 — 正确
- `(2)` **UB** — 声明 3 个参数但只传 2 个，第三个 `va_arg` 读到栈上垃圾数据
- `(3)` **UB** — `2.0` 是 `double`（8 字节），`va_arg(ap, int)` 按 `int`（4 字节）读 → 数据截断/错位

**教训：** 可变参数没有类型检查。用 `format` 属性让编译器帮忙（`__attribute__((format(printf, ...)))`）。

**复习：** → [7.6 Variable Argument Lists](./7.6-variable-argument-lists/7.6-variable-argument-lists.md)

</details>
