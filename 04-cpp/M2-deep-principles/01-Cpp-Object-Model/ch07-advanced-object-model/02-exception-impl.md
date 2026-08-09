# 7.2 异常处理实现

> 第 7 章 · 上一节：[7.1 模板实例化](01-template-instantiation.md) · 下一节：[7.3 模板+异常交互](03-template-exception.md)

## 这节讲什么

现代 C++ 用 table-based EH：编译器为每个函数生成异常表（.gcc_except_table），记录哪些区间可被哪些 catch 捕获。正常路径零开销，异常路径昂贵。

---

## 为什么要学这个（先建立直觉）

C 程序员用 setjmp/longjmp 做非局部跳转——但 longjmp 不析构对象：

```c
#include <setjmp.h>
jmp_buf env;
void risky_func() {
    Widget_C w;  // C 结构体（无析构）
    if (error) longjmp(env, 1);  // 跳回 setjmp 处
    // w 不会被清理（C 没有析构函数）
}
```

C++ 的 table-based EH 更先进——正常路径零开销，异常路径自动析构：

```cpp
void risky_func() {
    Widget w;  // C++ 对象（有析构）
    if (error) throw std::runtime_error("fail");
    // 如果抛异常 → w 自动析构（栈展开）
}
// 正常路径：无额外指令（异常表在只读段）
// 抛异常：查表 → 栈展开 → 析构 w → 跳转
```

---

## Table-based EH 详解

### 异常表的结构

```
.gcc_except_table 段（只读数据）：

函数 foo() 的异常表：
| PC 范围          | catch 类型      | 需析构的局部对象 |
|-----------------|----------------|----------------|
| 0x1000-0x1010   | runtime_error  | Widget w       |
| 0x1010-0x1020   | (无 catch)     | Widget w       |
| 0x1020-0x1030   | (cleanup only) | (无)           |

正常执行时不查表 → 零开销
抛异常时查表 → 定位 handler + 析构
```

### 抛异常的流程

```
throw std::runtime_error("fail");
  1. 构造异常对象（栈上或特殊区域）
  2. 调 __cxa_throw（运行时库）
  3. 查当前函数的异常表 → 找 catch handler
     3a. 找到 → 栈展开到 catch 位置 → 析构沿途局部对象 → 跳转
     3b. 没找到 → 弹栈到调用者 → 查调用者的异常表 → 重复
  4. 直到找到匹配的 catch 或 std::terminate
```

### 对比 setjmp/longjmp

```cpp
// setjmp/longjmp（C 方式，C++ 也可用但不推荐）
#include <csetjmp>
jmp_buf env;
void risky() {
    Widget w;
    if (error) longjmp(env, 1);  // w 不析构！资源泄漏
}

// table-based EH（C++ 方式）
void risky() {
    Widget w;
    if (error) throw std::runtime_error("fail");
    // w 自动析构（栈展开）
}
```

---

## 常见错误（新手踩坑）

### 错误 1：用 setjmp/longjmp 替代异常

```cpp
#include <csetjmp>
jmp_buf env;
void func() {
    std::string s = "hello";
    if (error) longjmp(env, 1);  // s 不析构 → 内存泄漏
}
// C++ 中不要用 longjmp——用异常替代
```

### 错误 2：以为异常表影响正常路径性能

```cpp
// 异常表在 .gcc_except_table 只读段
// 正常路径不查表 → 零开销
// 但二进制体积增大（异常表占空间）
```

### 错误 3：析构函数里 longjmp

```cpp
~Widget() {
    if (error) longjmp(env, 1);  // 灾难！
    // 如果在栈展开期间 longjmp → 跳过其他析构 → 资源泄漏
    // 比 throw 还危险
}
```

---

## 和 C 的区别

| 特性 | C setjmp/longjmp | C++ table-based EH |
|------|-----------------|-------------------|
| 正常路径开销 | setjmp 有开销（保存寄存器） | **零**（异常表不执行） |
| 析构对象 | **不析构** | 自动析构（栈展开） |
| 类型安全 | 无（int 返回码） | 有（异常类型匹配） |
| 二进制 | 小 | 大（异常表） |
| 可关闭 | N/A | `-fno-exceptions` |

---

## HFT 关联

1. **零开销模型的原理**：异常表在只读段，正常路径不执行——HFT 热路径可以安全使用 try/catch（不抛异常时零开销）。
2. **`-fno-exceptions` 移除异常表**：关异常后 .gcc_except_table 段被移除，二进制减小——但 throw/catch 不可用。
3. **不用 longjmp**：C++ 中 longjmp 不析构对象 → 资源泄漏。用异常或错误码替代。

---

## 代码自测

### Q1: 正常路径开销

```cpp
int process(int x) {
    try {
        if (x < 0) throw std::invalid_argument("negative");
        return x * 2;
    } catch (const std::invalid_argument& e) {
        return -1;
    }
}
int result = process(42);  // 正常路径
// 有异常相关开销吗？
```

<details>
<summary>答案与复习指引</summary>

正常路径（x >= 0）几乎零开销。异常表在 .gcc_except_table 只读段，不执行。只有一条条件跳转（`if (x < 0) goto throw`）。catch 块在正常路径不执行。这就是零开销模型。

**复习：** → [7.2 异常处理实现](./02-exception-impl.md)
</details>

### Q2: 栈展开

```cpp
struct Logger {
    ~Logger() { cout << "dtor "; }
};
void foo() {
    Logger l1;
    Logger l2;
    throw std::runtime_error("fail");
}
// 抛异常时 l1 和 l2 的析构顺序？
```

<details>
<summary>答案与复习指引</summary>

先 `l2` 后 `l1`（声明逆序）。栈展开时按构造的逆序析构局部对象——和正常离开作用域的顺序一样。这就是 table-based EH 比 longjmp 的优势：自动析构，零泄漏。

**复习：** → [7.2 异常处理实现](./02-exception-impl.md)
</details>

### Q3: longjmp vs throw

```cpp
#include <csetjmp>
jmp_buf env;
void func_c() {
    std::string s = "data";  // 有析构
    longjmp(env, 1);  // s 会析构吗？
}
void func_cpp() {
    std::string s = "data";
    throw std::runtime_error("fail");  // s 会析构吗？
}
```

<details>
<summary>答案与复习指引</summary>

`func_c`：s **不析构**（longjmp 直接跳转，不执行析构）→ 内存泄漏。`func_cpp`：s **自动析构**（throw 触发栈展开）→ 零泄漏。C++ 中绝不用 longjmp——用 throw/catch 替代。

**复习：** → [7.2 异常处理实现](./02-exception-impl.md)
</details>

### Q4: -fno-exceptions

```bash
# -fno-exceptions 对二进制有什么影响？
# 对正常路径性能有什么影响？
```

<details>
<summary>答案与复习指引</summary>

二进制：移除 .gcc_except_table 段 + 异常相关代码 → 减小体积。正常路径性能：几乎无影响（异常表本来就不执行）。主要好处是二进制更小 + 确定性更好（确保不会意外抛异常）。但失去 STL 异常保证（如 new 失败行为变化）。

**复习：** → [7.2 异常处理实现](./02-exception-impl.md)
</details>

---

## 参考与延伸

- 下一节：[7.3 模板+异常交互](03-template-exception.md)
- 回到：[第 7 章 高级对象模型](README.md)
