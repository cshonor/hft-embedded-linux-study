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

<details>
<summary>参考答案</summary>

1. placement new 在旧对象存储上重建对象后，旧指针/旧变量名在编译器眼里**仍指代原来的那个对象**。若原类型含 const 成员（或引用成员），编译器被允许假定这些成员的值在整个生命周期内不变，于是它可能**常量折叠/缓存旧值**，读到的不是新写入的值——这属于未定义行为，而且往往只在开优化时才暴露。
正确做法是使用 placement new 返回的指针，或用 `std::launder` 把旧指针"刷新"为指向新对象的指针。
2. 因为标准规定：const 对象（以及类类型中的 const 非静态数据成员、引用成员）在其生命周期内**值不可改变**。这条规则本身就是给优化器的许可证——既然值不会变，就可以把它当常量处理（寄存器缓存、常量传播、去重）。
所以"用 placement new 改掉一个含 const 成员的对象"在对象模型层面不是「修改」，而是「结束旧对象生命期、创建新对象」；旧的名字/指针指向的仍是旧对象，编译器按旧对象的不变性做优化也就顺理成章。
3. 因为 `reinterpret_cast` 只是**重新解释地址类型**，并不创建或"指向"一个对象：编译器看到的是一个 `unsigned char[]` 的存储，通过该指针访问 `Widget` 违反了对象模型/类型规则，属于未定义行为。
正确做法是：要么使用 placement new **返回的** `Widget*`（它确实指向新创建的对象）；要么在只有 `buf` 的情况下用 `std::launder(reinterpret_cast<Widget*>(buf))` 明确告诉编译器"这个地址上现在存在一个 `Widget` 对象"。
4. 不需要 launder 的典型情况：
   - 普通 `new` / placement new **返回的指针**——它天然指向新对象；
   - 访问**非 const、非引用**的普通成员：符合标准 [basic.life] 的"透明可替换"规则，旧名字可直接指代新对象；
   - STL 容器内部（`vector`/`optional` 等已由实现处理好），用户无需操心；
   - 在正确类型的存储上 placement new 后直接使用返回值。
一句话：只要你手上的指针确实"指向当前那个对象"，就不需要 launder。
5. **运行期零开销**：`std::launder` 是 `constexpr noexcept`，实现就是 `return p;`，通常被完全优化掉，不产生任何指令。
它的本质是**编译期的优化屏障 / provenance 提示**：告诉优化器"这个地址上的对象可能已经不是你以为的那个了"，从而禁止它基于旧对象的 const 值、旧类型、旧成员布局做常量折叠与值缓存。换言之，它改的是编译器的推理，而不是运行时的地址。

</details>
