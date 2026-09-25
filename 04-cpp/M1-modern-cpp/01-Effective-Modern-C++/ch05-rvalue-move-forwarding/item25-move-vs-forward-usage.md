# Item 25：对万能引用用 std::forward，对右值引用用 std::move

> 第 5 章 · Item 25 · 上一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 为什么要学这个（先建立直觉）

C 没有引用的概念——所有传参要么按值（拷贝），要么传指针。C++ 有左值引用、右值引用、万能引用，混用 `move` 和 `forward` 会导致严重 bug。

核心场景：你在模板函数里收到一个参数 `x`，要把它传给另一个函数。问题是——`x` 在函数内部是左值（有名字的就是左值），不管它声明时是 `T&&` 还是 `T&`。你需要用 `forward` 或 `move` 恢复它的原始左右值性。

```cpp
// 万能引用：x 可能是左值也可能是右值
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 保留原始性

// 右值引用：x 一定是右值（但函数内是左值！）
void take(Widget&& w) { target(std::move(w)); }  // 无条件转右值
```

**致命错误：** 对万能引用用 `std::move`——如果实参是左值，`move` 会无条件搬走它，调用方的左值被掏空。

---

## 这节讲什么

混用 `move` 和 `forward` 会导致意外拷贝或意外移动。规则很简单：万能引用用 `forward`，右值引用用 `move`。

---

## 核心规则

### 万能引用 → forward

```cpp
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 万能引用 → forward

std::string s = "hello";
set(s);               // T = string& → forward 保留左值 → target(string&)
set(std::string("x")); // T = string → forward 转右值 → target(string&&)
```

### 右值引用 → move

```cpp
void take(Widget&& w) { target(std::move(w)); }  // 右值引用 → move
// w 在函数内是左值（有名字），但声明为右值引用 → 用 move 恢复右值性
```

### 致命错误：对万能引用用 move

```cpp
template<class T>
void bad(T&& x) { target(std::move(x)); }  // 危险！

std::string s = "important data";
bad(s);  // T = string& → x 是左值引用
// std::move(x) 无条件转右值 → s 被掏空！
std::cout << s;  // s 是空字符串！
```

### 最后一次使用可以 move

```cpp
template<class T>
void set_two(T&& x) {
    target1(x);                    // 第一次使用：不 move
    target2(std::forward<T>(x));   // 最后一次：forward（或 move 如果是右值引用）
}
```

---

## 常见错误（新手踩坑）

**错误 1：万能引用用了 move**
```cpp
template<class T>
void add(T&& item) {
    storage.push_back(std::move(item));  // 左值实参被掏空！
}
std::string s = "data";
add(s);  // s 被掏空！
```
**修正：** `storage.push_back(std::forward<T>(item));`

**错误 2：右值引用用了 forward**
```cpp
void process(Widget&& w) {
    target(std::forward<Widget>(w));  // 能用但不直观
    // 右值引用一定是右值，用 move 更清晰
}
```
**修正：** `target(std::move(w));`——语义更明确。

**错误 3：return 时用 move 阻止 NRVO**
```cpp
template<class T>
T make() {
    T result;
    return std::move(result);  // 阻止 NRVO！
}
```
**修正：** `return result;`——让编译器做返回值优化。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 参数传递 | 按值/指针 | 值/引用/右值引用/万能引用 | C++ 有引用和移动语义 |
| "搬走"标记 | 手动交换+置空 | `move`（右值引用）/ `forward`（万能引用） | 类型安全 |
| 转发 | 不适用 | `forward<T>` 保留左右值性 | 完美转发 |

**口诀：** 万能引用配 `forward`，右值引用配 `move`。不确定是哪种引用？回到 Item 24 的判断标准（有 `T&&` + 类型推导 = 万能引用）。

---

## HFT 关联

- **回调转发**：`template<class F> void onEvent(F&& cb) { storage.push_back(std::forward<F>(cb)); }` 保留 cb 的左右值性，避免不必要的拷贝。
- **订单构造**：`template<class... Args> void emplace_order(Args&&... args) { orders.emplace_back(std::forward<Args>(args)...); }` 完美转发构造参数。
- **配置传递**：策略配置用万能引用接收，`forward` 到内部存储——避免拷贝大配置对象。

---

## 自测题

1. 对万能引用用 `std::move` 为什么危险？
2. 什么时候用 `std::move`？什么时候用 `std::forward`？
3. `std::forward<T>(x)` 为什么必须显式传 `T`？
4. 函数内右值引用参数为什么是左值？需要用什么恢复？
5. 下面代码有什么问题？
```cpp
template<class T>
void store(T&& x) {
    cache = std::move(x);
}
std::string config = "important";
store(config);
std::cout << config;
```

<details>
<summary>参考答案</summary>

1. 因为 `std::move` 会**无条件**把实参转成右值，而万能引用在接左值时 `T` 被推为 `T&`、`x` 本身是个左值引用。`std::move(x)` 会强行把这左值也变成右值，导致目标函数选中移动构造/移动赋值——于是调用方传入的左值对象被**掏空**（进入有效但未指定状态），而调用方完全没料到"我只是传了个参数"却被搬走了内容。这是最隐蔽的一类 bug：编译通过、运行不崩、数据悄悄丢了。万能引用必须用 `std::forward<T>` 才能保留原始值类别。

2. 用 `std::move`：形参是**右值引用**（`void f(Widget&& w)`），或对象是明确的局部变量/即将销毁、你确定不再需要它时（如 `push_back(std::move(local))`、实现移动构造函数），语义是"转移所有权"。用 `std::forward<T>`：形参是**万能引用**（`template<class T> void f(T&& x)`），需要把实参原样传给下一层，语义是"保留原值类别"。口诀：**右值引用 → move；万能引用 → forward**。另外返回局部对象时两者都不要用，让 NRVO/RVO 生效。

3. 因为 `std::forward` 的"条件转换"依赖模板参数 `T` 携带的原始值类别信息：万能引用接左值时 `T` 推为 `T&`，接右值时 `T` 推为 `T`。`std::forward<T>` 通过判断 `T` 是不是引用类型来决定是否转成右值；如果写成 `std::forward(x)` 而不显式指定 `T`，`T` 无从获取（它不是可推导的函数实参类型，必须显式提供），编译器就会报错或推错。这也是 `forward` 与 `move` 在调用写法上的显著区别——`move` 不需要模板实参，`forward` 必须写。

4. 因为**具名的右值引用本身是左值**：`void f(Widget&& w)` 里 `w` 是一个有名字的变量，可以取地址、可以多次使用，按值类别规则它是左值表达式。如果不做处理就把 `w` 传给下一层，会选中**拷贝**重载而不是移动重载——移动语义在"透传"时丢失。恢复方法是 `std::move(w)`（无条件转回右值）；若形参是万能引用则用 `std::forward<T>(w)`。

5. 与 Item 23 的坑相同：`x` 是万能引用，传左值 `config` 时 `T` 推为 `std::string&`，`std::move(x)` 无条件转成右值，`cache = ...` 走移动赋值，**把 `config` 掏空**，随后的 `std::cout << config` 输出未指定内容（常见实现下是空串）。修正：
```cpp
template<class T> void store(T&& x) { cache = std::forward<T>(x); }
```
用 `forward` 后，传左值走拷贝赋值（`config` 保持 "important"），传右值才走移动赋值。

</details>

---

## 参考与延伸

- 下一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)
- 回到：[第 5 章](README.md)
