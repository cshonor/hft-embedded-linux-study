# 第 3 章 数据语义

**The Semantics of Data**

## 本章讲什么

类的数据成员在内存中如何排列？单继承、多继承、虚继承下成员布局有何不同？`sizeof` 与 `offsetof` 的真实结果受什么影响？本章讲数据布局规则——这是预测 cache 行为与跨语言 ABI 的基础。

## 要点

### 成员布局规则

- 非 static 成员按**声明顺序**排列，中间可能插入 **padding**（对齐要求）。
- static 成员不在对象内。
- 编译器可能自由安排 padding（C++ 标准不保证布局顺序跨编译器一致，但实际多数按声明序）。

### 继承布局

- **单继承**：基类成员在前，派生成员追加其后；共用一个 vptr（派生复用基类的 vptr 位置）。
- **多继承**：多个基类子对象依次排列，各含自己的 vptr；访问非首基类成员需 this 调整。
- **虚继承**：虚基类子对象只存一份（解决菱形继承），通过虚基类指针/偏移表定位——额外间接。

### `sizeof` 的真相

`sizeof(Derived)` = 各基类子对象 + 自身成员 + padding + vptr 数量。虚继承会显著增大对象（偏移表指针）。

### 指向数据成员的指针

```cpp
int Point::* p = &Point::x;  // 指向成员的指针，本质是偏移量
obj.*p = 10;
```
指向数据成员的指针是**偏移量**而非真实地址——通过 `obj.*p` 在具体对象上定位。虚继承下偏移计算更复杂。

## HFT 关联

- **布局影响 cache**：`sizeof` + 布局决定 cache 行能装几个对象。虚继承让对象膨胀，热路径数据结构避免虚继承。
- **字段重排减 padding**：合理排列成员（大对齐在前）减少 padding，`sizeof` 缩小，cache 友好（与《C 和指针》ch10 结构体对齐同理）。
- **POD 布局可 `memcpy`**：POD 类型布局可预测，跨进程共享内存安全；非 POD（有 vptr）不能 `memcpy`。

## 自测题

1. 单继承和多继承的成员布局有何不同？多继承为什么需要 this 调整？
2. 虚继承如何解决菱形继承？它的布局代价是什么？
3. 指向数据成员的指针本质是什么？
4. 为什么 HFT 热路径数据结构避免虚继承？
5. POD 与非 POD 在 `memcpy` 安全性上有什么区别？

## 代码自测

### Q1: 空基类优化（EBO）
```cpp
class Empty {};
class A : public Empty { int x; };      // 继承空类
class B { Empty e; int x; };             // 组合空类
```
> `sizeof(A)` 和 `sizeof(B)` 分别是多少？为什么不同？

<details>
<summary>答案与复习指引</summary>

- `sizeof(A)` = **4**（空基类优化，Empty 不占空间）
- `sizeof(B)` = **8**（Empty 作为成员需要 1 字节 + 对齐填充到 4）

EBO（Empty Base Optimization）：空类作为基类时编译器可优化为 0 字节，但作为成员时 C++ 标准要求不同对象地址唯一，至少 1 字节。这就是 STL 用继承而非组合传递 `std::less<T>` 等空仿函数的原因。

**复习：** → [空基类优化](./README.md)
</details>

### Q2: 对齐与布局
```cpp
struct BadLayout {
    char c;     // 1
    double d;   // 8
    char c2;    // 1
};
struct GoodLayout {
    double d;   // 8
    char c;     // 1
    char c2;    // 1
};
```
> `sizeof(BadLayout)` 和 `sizeof(GoodLayout)` 分别是多少？为什么？

<details>
<summary>答案与复习指引</summary>

- `sizeof(BadLayout)` = **24**（c=1 + 7填充 + d=8 + c2=1 + 7填充）
- `sizeof(GoodLayout)` = **16**（d=8 + c=1 + c2=1 + 6填充）

double 需 8 字节对齐，BadLayout 中 c 后需填充 7 字节才能放 d，c2 后也需填充到 8 的倍数。GoodLayout 把 double 放前面，两个 char 紧挨着，只末尾填充。

**HFT 关联**：对象大小直接影响 cache 行利用率，布局优化减少 false sharing 和 cache miss。

**复习：** → [对齐与布局](./README.md)
</details>
