# launder 解决的问题

## placement new 与编译器假设

```cpp
struct X {
    const int n;  // const 成员
    X(int v) : n(v) {}
};

X x{1};
const int* p = &x.n;     // p 指向 x.n，值为 1
new (&x) X{2};            // placement new：原地把 x 重建为 n=2

// *p 是什么？
std::cout << *p;  // C++17 前：UB！可能输出 1（编译器缓存）
                  // C++17 前：编译器假设 const 成员不变
```

## 为什么编译器会出错

```cpp
// 编译器看到：
// 1. x.n 是 const → 假设不变
// 2. p 指向 x.n → *p == 1
// 3. 优化：把 *p 替换为常量 1

// 但 placement new 改了 x.n → 编译器假设错误
// *p 实际是 2，但编译器可能输出 1（缓存在寄存器中）
```

## 其他需要 launder 的场景

```cpp
// 1. 引用成员
struct Y { int& ref; };
Y y{some_int};
new (&y) Y{other_int};
// y.ref 仍指向 some_int（引用绑定后不可变）
// launder 也不能修引用成员，但能安全访问新对象

// 2. 通过 unsigned char buffer 构造
alignas(Widget) unsigned char buf[sizeof(Widget)];
new (buf) Widget(42);

// 直接 cast：编译器可能假设 buf 是 char 数组，不是 Widget
Widget* p1 = reinterpret_cast<Widget*>(buf);  // UB（C++17 前）

// launder：告诉编译器 buf 上有新 Widget
Widget* p2 = std::launder(reinterpret_cast<Widget*>(buf));  // OK
```

## 不需要 launder 的场景

```cpp
// 普通 new：编译器知道是新对象
auto* p = new Widget(42);  // 不需要 launder

// 非 const 成员的 placement new：通常不需要
struct Z { int n; };  // 非 const
Z z{1};
new (&z) Z{2};
// z.n 不是 const，编译器不假设不变
// 但严格来说仍建议 launder

// vector 内部：已处理
std::vector<Widget> v;
v.resize(10);  // vector 内部用 launder，用户不用管
```

## launder 的本质

```cpp
template <typename T>
[[nodiscard]] constexpr T* launder(T* p) noexcept;
// 运行时：零开销（就是 return p）
// 编译时：告诉优化器"这个指针指向的对象可能和之前不同"
//         → 禁止基于旧值的优化
```

**本质**：launder 是编译器屏障，不是运行时操作。它"清洗"指针，消除编译器对指针指向对象的假设。

## 自测题

1. placement new 重建 const 成员后，直接用旧指针有什么问题？
2. 为什么编译器会假设 const 成员不变？
3. 通过 `unsigned char buf` 构造对象后，为什么不能直接 `reinterpret_cast`？
4. 什么场景不需要 launder？
5. launder 的运行时开销是什么？本质是什么？
