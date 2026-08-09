# 第 18 章 用于大型程序的工具

面向**中大型项目、多文件工程、复杂依赖**的 C++ 进阶机制，写小 demo 基本用不到。

## 小节

- [18.1 异常处理（深入版）](./18.1-异常处理（深入版）.md)
- [18.2 命名空间 namespace](./18.2-命名空间namespace.md)
- [18.3 多重继承与虚继承](./18.3-多重继承与虚继承.md)
- [18.4 运行时类型识别 RTTI](./18.4-运行时类型识别RTTI.md)
- [学习优先级](./18.5-学习优先级.md)


## 章节摘要

大型程序工具：异常处理（深入版）、命名空间（`namespace`）、多重继承与虚继承、运行时类型识别（RTTI：`dynamic_cast`/`typeid`）。

### 和 C 的区别

| C | C++ |
|---|-----|
| `setjmp`/`longjmp` | `try`/`catch`/`throw`（栈展开+RAII） |
| 前缀避免命名冲突 `mylib_func` | `namespace mylib { func }` |
| 无多重继承 | 多继承/虚继承 |
| 无运行时类型 | `dynamic_cast`/`typeid`（RTTI） |

## 章节自测

### Q1: 异常安全与 RAII

```cpp
void process() {
    std::vector<int> v(1000);
    std::ifstream f("data.txt");
    // ... 处理 ...
    throw std::runtime_error("error");
    // v 和 f 会泄漏吗？
}
```

> `throw` 后 `v` 和 `f` 会泄漏吗？为什么？

<details>
<summary>答案与复习指引</summary>

**不会泄漏。** 异常抛出后栈展开（stack unwinding）——所有局部对象的析构函数自动调用。`v` 的析构释放内存，`f` 的析构关闭文件。这就是 RAII 的核心优势。

**和 C 的区别：** C 的 `longjmp` 不调用析构函数（C 也没有析构），跳转路径上的资源全泄漏。C++ 异常保证栈展开时 RAII 对象自动清理。

**教训：** 用 RAII 管理资源（智能指针/容器/文件流），异常安全自然得到。

**复习：** → [18.1 异常处理（深入版）](./18.1-异常处理（深入版）.md)
</details>

### Q2: namespace

```cpp
namespace math {
    int abs(int x) { return x < 0 ? -x : x; }
}
namespace physics {
    int abs(int x) { return x < 0 ? -x : x; }  // 不同含义
}
int main() {
    std::cout << math::abs(-5) << " " << physics::abs(-5);
}
```

> 输出是什么？`namespace` 解决了什么问题？

<details>
<summary>答案与复习指引</summary>

**输出：** `5 5`

**`namespace` 解决的问题：** 命名冲突。不同库可以有同名函数/类，通过命名空间区分。C 用前缀（`math_abs`/`physics_abs`）绕开冲突，丑且易错。

**`using` 声明：** `using math::abs;` 引入特定名称；`using namespace math;` 引入整个命名空间（慎用，可能引入意外冲突）。

**复习：** → [18.2 命名空间 namespace](./18.2-命名空间namespace.md)
</details>

### Q3: dynamic_cast

```cpp
class Base { public: virtual ~Base() {} };
class Derived : public Base { public: void special() { std::cout << "special"; } };
Base *b = new Derived;
Derived *d = dynamic_cast<Derived*>(b);
if (d) d->special();
```

> 输出是什么？`dynamic_cast` 失败时返回什么？引用版本的 `dynamic_cast` 失败时呢？

<details>
<summary>答案与复习指引</summary>

**输出：** `special`

**失败行为：**
- 指针版本：返回 `nullptr`
- 引用版本：抛 `std::bad_cast` 异常（引用不能为空）

**前提：** `dynamic_cast` 只对**多态类型**（有虚函数的类）有效——靠 vtable 里的 type_info 运行时检查类型。非多态类型用 `dynamic_cast` 编译错误。

**代价：** 运行时类型查找有开销（cache miss + 字符串比较），HFT 热路径避免使用。

**复习：** → [18.4 运行时类型识别 RTTI](./18.4-运行时类型识别RTTI.md)
</details>

### Q4: 多重继承

```cpp
class A { public: int a; };
class B { public: int b; };
class C : public A, public B { public: int c; };
C obj;
obj.a = 1; obj.b = 2; obj.c = 3;
// sizeof(C) 是多少（假设 int=4）？
```

> `sizeof(C)` 是多少？多重继承的布局是什么？

<details>
<summary>答案与复习指引</summary>

**`sizeof(C) = 12`**（三个 int，通常无 padding）

**布局：** `C` 对象内存中依次排列 `A` 子对象（`int a`）、`B` 子对象（`int b`）、`C` 自身成员（`int c`）。

**多重继承的代价：**
1. 如果 `A`/`B` 有虚函数，各有自己的 vptr → 对象更大
2. 基类指针转换需要 this 调整：`B *pb = &obj;` 的 `pb` 不指向 `obj` 开头，而是指向 `B` 子对象的位置
3. 菱形继承（A→B, A→C, D:B,C）导致 A 的数据存两份——用虚继承解决

**复习：** → [18.3 多重继承与虚继承](./18.3-多重继承与虚继承.md)
</details>
