# 第一部分 基础核心（Basics）

进阶必懂基础误区，全书前置地基。

## 条款

- [条款 1：分清指针（*）和引用（&）的本质区别、使用场景边界](./item01-分清指针（）和引用（&）的本质区别、使用场景边界.md)
- [条款 2：优先使用 C++ 新式强制类型转换（static_cast/dynamic_cast 等），摒弃 C 风格强转](./item02-优先使用C++新式强制类型转换（static_castdynamic_cast等），摒弃C风格强转.md)
- [条款 3：绝对不要对数组做多态处理（经典大坑）](./item03-绝对不要对数组做多态处理（经典大坑）.md)
- [条款 4：避免无意义的默认构造函数，理解默认构造的隐性开销与风险](./item04-避免无意义的默认构造函数，理解默认构造的隐性开销与风险.md)


## 章节摘要

基础：指针 vs 引用、优先 C++ cast、不对数组做多态处理、避免无意义默认构造。

## 代码自测

### Q1: 指针 vs 引用

```cpp
int x = 42;
int *p = &x;
int &r = x;
// p 可以为 nullptr 吗？r 呢？
// p 可以重新指向别的对象吗？r 呢？
```

> 指针和引用的三个本质区别是什么？

<details>
<summary>答案与复习指引</summary>

1. **可空性**：指针可以为 `nullptr`，引用不可为空（必须绑定到有效对象）
2. **可重绑**：指针可以重新指向别的对象，引用绑定后不可改变
3. **语法**：指针用 `*p` 解引用，引用直接用 `r`（像变量名一样）

**选择标准：** 必须"总指向某对象"且"不改变指向"时用引用；需要"可能为空"或"需要改变指向"时用指针。

**函数参数：** 不接受空的参数用引用（`void f(string &s)`），可能传空的用指针（`void f(string *s)`）。

**复习：** → [条款 1：分清指针和引用](./item01-分清指针（）和引用（&）的本质区别、使用场景边界.md)
</details>

### Q2: 数组多态灾难

```cpp
class Base { public: virtual ~Base() {} };
class Derived : public Base { int extra[100]; };
void delete_array(Base *arr) {
    delete[] arr;  // 会怎样？
}
Base *arr = new Derived[10];
delete_array(arr);
```

> `delete[] arr` 会发生什么？

<details>
<summary>答案与复习指引</summary>

**UB / 崩溃。** `delete[]` 通过指针类型（`Base*`）计算元素大小。`Base` 可能 8 字节（vptr），`Derived` 408 字节。`delete[]` 按 8 字节步长遍历析构——只析构了第一个元素的前 8 字节，后面的 `Derived` 析构错位。

**根因：** 数组元素的 `delete[]` 依赖指针类型的 `sizeof`。多态数组中，基类指针的 `sizeof` != 实际元素大小。

**规则：** 绝不对数组做多态处理。如果要存多态对象，用 `vector<unique_ptr<Base>>`。

**复习：** → [条款 3：绝对不要对数组做多态处理](./item03-绝对不要对数组做多态处理（经典大坑）.md)
</details>
