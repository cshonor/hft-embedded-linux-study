# 8.3 函数适配器
> 第 8 章 适配器 · 第 3 节 · 上一节：[8.2 迭代器适配器](02-iterator-adapters.md) · 下一节：[回到目录](../README.md)

## 为什么要学这个（先建立直觉）

C 里"绑定参数"或"包装成员函数"完全靠手写：

```c
// C: 想把 less_than 的第二个参数固定为 5
int less_than_5(int x) { return x < 5; }  // 手写一个新函数
// 想调用对象的成员函数？手动取地址 + 传 this
```

C++03 的函数适配器提供了"绑定"和"包装"的机制：

```cpp
// C++03: bind2nd 绑定参数
std::bind2nd(std::less<int>(), 5)  // x < 5
// mem_fun 包装成员函数
std::mem_fun(&Shape::draw)
```

C++11 后 `std::bind` + lambda 全面取代了这套机制。理解函数适配器的历史，你才能读懂老代码和 STL 源码。

## 这节讲什么

函数适配器包装可调用对象，实现参数绑定、取反、成员函数包装。

### 三类函数适配器

| 类别 | C++03 适配器 | C++11+ 替代 | 用途 |
|------|------------|-----------|------|
| 参数绑定 | `bind1st`/`bind2nd` | `std::bind`/lambda | 固定参数 |
| 谓词取反 | `not1`/`not2` | `std::not_fn`/lambda | 逻辑取反 |
| 成员函数 | `mem_fun`/`mem_fun_ref` | `std::mem_fn`/lambda | 包装成员函数 |
| 函数指针 | `ptr_fun` | 不需要 | 函数指针→仿函数 |

### bind1st / bind2nd（参数绑定）

```cpp
// bind2nd: 固定第二个参数
std::bind2nd(std::less<int>(), 5)
// → [x](x) { return x < 5; }

std::bind1st(std::less<int>(), 5)
// → [x](x) { return 5 < x; }  即 x > 5

// 用法
std::vector<int> v = {1, 3, 5, 7, 9};
auto it = std::find_if(v.begin(), v.end(),
    std::bind2nd(std::greater<int>(), 5));
// 找第一个 > 5 → 指向 7
```

### not1 / not2（谓词取反）

```cpp
// not1: 否定一元谓词
std::not1(std::bind2nd(std::less<int>(), 5))
// → !(x < 5) = x >= 5

// not2: 否定二元谓词
std::not2(std::less<int>())
// → !(a < b) = a >= b

// 用法
std::sort(v.begin(), v.end(), std::not2(std::less<int>()));
// 降序排序（但用 greater 更清晰）
```

### mem_fun / mem_fun_ref（成员函数包装）

```cpp
class Shape {
public:
    void draw() const { std::cout << "draw\n"; }
    double area() const { return 0; }
};

std::vector<Shape*> shapes;
// 想对每个 Shape* 调用 draw()
std::for_each(shapes.begin(), shapes.end(),
    std::mem_fun(&Shape::draw));  // C++03

std::vector<Shape> shapes2;
std::for_each(shapes2.begin(), shapes2.end(),
    std::mem_fun_ref(&Shape::draw));  // 对象而非指针

// C++11: std::mem_fn 统一了两者
std::for_each(shapes.begin(), shapes.end(),
    std::mem_fn(&Shape::draw));  // 指针和对象都行

// C++11 lambda: 最简洁
std::for_each(shapes.begin(), shapes.end(),
    [](Shape* s) { s->draw(); });
```

`mem_fun` vs `mem_fun_ref` 的区别：
- `mem_fun`：容器存储**指针**（`Shape*`），通过指针调用
- `mem_fun_ref`：容器存储**对象**（`Shape`），直接调用

### ptr_fun（函数指针→仿函数）

```cpp
// C: 函数指针
int str_len(const std::string& s) { return s.length(); }

// ptr_fun 把函数指针包装成仿函数（有关联类型）
std::ptr_fun(str_len)
// 这样适配器（not1/bind2nd）就能包装它

// C++11: 完全不需要 ptr_fun
// 函数指针可以直接传给算法
std::for_each(v.begin(), v.end(), str_len);  // 直接用
```

### C++11 后的全面替代

```cpp
// C++03: 一串适配器
std::not1(std::bind2nd(std::less<int>(), 5))

// C++11: lambda
[](int x) { return x >= 5; }

// C++11: std::bind
std::bind(std::greater<int>(), std::placeholders::_1, 5)

// C++17: not_fn
std::not_fn([](int x) { return x < 5; })
```

| C++03 | C++11+ | 推荐度 |
|-------|--------|--------|
| `bind2nd(op, 5)` | `[](auto x) { return op(x, 5); }` | lambda |
| `not1(pred)` | `std::not_fn(pred)` 或 `!pred` | not_fn/lambda |
| `mem_fun(&C::f)` | `std::mem_fn(&C::f)` 或 `[](auto* p) { p->f(); }` | lambda |
| `ptr_fun(f)` | 直接用 `f` | 不需要 |

## 常见错误（新手踩坑）

### 错误 1：C++17 后还在用 bind2nd

```cpp
// ❌ bind2nd 在 C++11 废弃，C++17 删除
std::bind2nd(std::less<int>(), 5);  // C++17 编译错误

// ✅ lambda
[](int x) { return x < 5; }
```

### 错误 2：混淆 mem_fun 和 mem_fun_ref

```cpp
std::vector<Shape*> ptrs;
std::vector<Shape> objs;

// ❌ 指针容器用 mem_fun_ref
std::for_each(ptrs.begin(), ptrs.end(),
    std::mem_fun_ref(&Shape::draw));  // 错误！应该用 mem_fun

// ✅ 指针用 mem_fun，对象用 mem_fun_ref
std::for_each(ptrs.begin(), ptrs.end(), std::mem_fun(&Shape::draw));
std::for_each(objs.begin(), objs.end(), std::mem_fun_ref(&Shape::draw));

// ✅✅ C++11: mem_fn 统一
std::for_each(ptrs.begin(), ptrs.end(), std::mem_fn(&Shape::draw));
std::for_each(objs.begin(), objs.end(), std::mem_fn(&Shape::draw));
```

### 错误 3：ptr_fun 在 C++11 已不需要

```cpp
// ❌ C++11 后不需要 ptr_fun
std::ptr_fun(str_len);  // 废弃

// ✅ 直接用函数指针
std::for_each(v.begin(), v.end(), str_len);
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写包装函数 | 适配器自动包装 | C++ 可组合 |
| 手动绑定参数 | `bind`/lambda | C++ 更灵活 |
| 手动调用成员函数 | `mem_fn`/lambda | C++ 泛型 |
| 无类型推导 | 模板自动推导 | C++ 类型安全 |

## HFT 关联

- **新代码用 lambda**：HFT 新代码一律用 lambda，可内联 + 可读 + 零开销
- **理解老代码**：量化系统老代码可能有 `bind2nd`/`not1`/`mem_fun` 链，理解才能维护
- **`std::function` 有开销**：不要用 `std::function` 做适配器——类型擦除 + 间接调用 + 可能堆分配
- **编译期组合**：lambda 组合是编译期的（闭包类型唯一），零运行时开销

## 代码自测

### Q1: bind2nd 和 bind1st 有什么区别？

```cpp
auto a = std::bind1st(std::less<int>(), 5);  // 固定第一个参数
auto b = std::bind2nd(std::less<int>(), 5);  // 固定第二个参数
// a(3) = ?
// b(3) = ?
```
> a(3) 和 b(3) 分别返回什么？

<details>
<summary>答案与复习指引</summary>

- `a(3)` = `(5 < 3)` = `false`（`bind1st` 固定第一个参数为 5）
- `b(3)` = `(3 < 5)` = `true`（`bind2nd` 固定第二个参数为 5）

**等价 lambda**：
```cpp
auto a = [](int x) { return 5 < x; };   // bind1st(less, 5)
auto b = [](int x) { return x < 5; };   // bind2nd(less, 5)
```

**记忆**：
- `bind1st(op, val)` → `op(val, x)` → 固定**第一个**参数
- `bind2nd(op, val)` → `op(x, val)` → 固定**第二个**参数

**复习：** → [bind1st / bind2nd](./03-function-adapters.md)
</details>

### Q2: not1 和 not2 的区别是什么？

```cpp
// not1: 否定一元谓词
auto neg1 = std::not1(std::bind2nd(std::less<int>(), 5));
// → !(x < 5) = x >= 5（一元）

// not2: 否定二元谓词
auto neg2 = std::not2(std::less<int>());
// → !(a < b) = a >= b（二元）
```
> 为什么 not1 不能否定二元谓词？

<details>
<summary>答案与复习指引</summary>

**not1** 生成一元否定谓词，需要萃取 `argument_type`（一个参数类型）。二元谓词有 `first_argument_type` 和 `second_argument_type`，没有 `argument_type`，所以 `not1` 对二元谓词编译失败。

**not2** 生成二元否定谓词，需要萃取 `first_argument_type` 和 `second_argument_type`。一元谓词没有这两个类型，所以 `not2` 对一元谓词编译失败。

**C++17 `not_fn`** 统一了两者——对任何元数的可调用对象都能工作（用可变参数模板 + 完美转发）。

**复习：** → [not1 / not2](./03-function-adapters.md)
</details>

### Q3: mem_fun 和 mem_fun_ref 的区别？

```cpp
std::vector<Shape*> ptrs;
std::vector<Shape> objs;

std::for_each(ptrs.begin(), ptrs.end(),
    std::mem_fun(&Shape::draw));      // 指针容器
std::for_each(objs.begin(), objs.end(),
    std::mem_fun_ref(&Shape::draw));  // 对象容器
```
> 为什么需要两个不同的适配器？C++11 怎么统一？

<details>
<summary>答案与复习指引</summary>

**区别**：
- `mem_fun(&C::f)`：生成调用 `p->f()` 的仿函数（通过**指针**调用）
- `mem_fun_ref(&C::f)`：生成调用 `obj.f()` 的仿函数（直接对**对象**调用）

**为什么需要两个**：C++03 的模板不能自动区分指针和对象，需要两个不同的适配器。

**C++11 `std::mem_fn`**：用 SFINAE/`decltype` 自动推导——指针调 `->`，对象调 `.`，一个适配器搞定：

```cpp
std::mem_fn(&Shape::draw)  // 对指针和对象都工作
```

**C++11 lambda 更直接**：
```cpp
[](Shape* p) { p->draw(); }     // 指针
[](Shape& s) { s.draw(); }       // 对象
[](auto& x) { x.draw(); }        // 通用（C++14）
```

**复习：** → [mem_fun / mem_fun_ref](./03-function-adapters.md)
</details>

### Q4: 为什么 C++11 后函数适配器被淘汰了？

```cpp
// C++03: bind2nd + not1 + mem_fun
std::not1(std::bind2nd(std::less<int>(), 5))
std::mem_fun(&Shape::draw)

// C++11: lambda
[](int x) { return x >= 5; }
[](Shape* s) { s->draw(); }
```
> lambda 相比函数适配器有哪些优势？

<details>
<summary>答案与复习指引</summary>

**lambda 的优势**：

| 优势 | 说明 |
|------|------|
| **可读性** | `[](int x) { return x >= 5; }` 比 `not1(bind2nd(less<int>(), 5))` 清晰百倍 |
| **无需基类** | 不需要继承 `unary_function`/`binary_function` |
| **可内联** | 闭包类型唯一，编译器可内联 `operator()` |
| **零开销** | 无适配器嵌套、无中间对象 |
| **万能** | 任何逻辑直接写，不受适配器组合限制 |
| **可调试** | 直接断点，类型名简单 |

**函数适配器被淘汰的原因**：
1. `bind1st`/`bind2nd` 只能固定一个参数，`std::bind`/lambda 更灵活
2. `not1`/`not2` 需要关联类型，lambda 不需要
3. `mem_fun`/`mem_fun_ref` 需要区分指针/对象，`mem_fn`/lambda 自动处理
4. `ptr_fun` 完全多余（函数指针直接可用）
5. 嵌套适配器可读性极差

**教训**：C++11+ 一律用 lambda。理解适配器只是为了读老代码和 STL 源码。

**HFT**：lambda 可内联 + 零开销 + 可读——热路径唯一选择。

**复习：** → [C++11 后的全面替代](./03-function-adapters.md)
</details>

## 参考与延伸

- 上一节：[8.2 迭代器适配器](02-iterator-adapters.md)
- 下一节：[回到目录](../README.md)
- 参考：Effective Modern C++ Item 34（优先 lambda 而非 bind）
