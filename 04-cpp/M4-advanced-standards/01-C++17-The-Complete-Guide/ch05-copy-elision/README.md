# 第 5 章 强制拷贝省略与未物化传值

**Mandatory Copy Elision or Passing Unmaterialized Objects**

## 本章讲什么

C++17 规定在某些情况下**必须省略**拷贝/移动——返回 prvalue 时不再"可选地"省略，而是语法上就不产生临时对象。这改变了"返回值必定拷贝"的旧观念。

## 要点

### C++17 之前：可选省略（NRVO/RVO）

```cpp
// C++14：编译器"可以"省略拷贝，但不保证
std::string make() {
    return std::string("hi");   // RVO：可能省略，可能不省
}
std::string s = make();   // 可能 0 次、1 次或 2 次构造
```

NRVO（Named RVO）和 RVO 是编译器优化，标准允许但不强制。关掉优化（`-fno-elide-constructors`）就会真的拷贝。

### C++17：强制省略（guaranteed copy elision）

C++17 规定：**返回 prvalue（纯右值）时，不产生临时对象**——不是"省略拷贝"，而是语法上根本没东西可拷贝。

```cpp
// C++17：返回 prvalue，零拷贝，语法保证
std::string make() {
    return std::string("hi");   // prvalue，直接在 s 的位置构造
}
std::string s = make();   // 1 次构造（在 s 处），0 次拷贝
```

关键区别：C++14 的 RVO 是"构造临时对象 + 可能省略拷贝"；C++17 是"根本不构造临时对象"。

### 影响：不可移动/不可拷贝的类型也能返回

```cpp
struct Immovable {
    Immovable() = default;
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
};

Immovable make() {
    return Immovable{};   // C++17 OK：无拷贝无移动
    // C++14 编译失败：需要拷贝/移动构造
}
```

`std::lock_guard`、`std::atomic` 这类不可拷贝不可移动的类型，C++17 可以直接返回。

### 传参也有未物化

```cpp
void foo(std::string s);   // 按值传参
foo(std::string("hi"));    // C++17：prvalue 直接在参数位置构造，零拷贝
```

### 不是所有场景都省略

强制省略只针对 **prvalue**。对 **lvalue**（有名变量）返回仍可能拷贝/移动（NRVO 仍是可选优化）：

```cpp
std::string make() {
    std::string s = "hi";
    return s;   // lvalue：NRVO 可选，可能移动
}
```

## HFT 关联

- **返回行情对象零拷贝**：`Tick make_tick()` 返回 prvalue，C++17 保证零拷贝直接在调用方构造。
- **不可移动类型工厂**：`atomic` 包装类、`lock_guard` 持有者这类不可移动对象，C++17 可安全工厂返回。
- **临时对象减少**：prvalue 不物化临时对象，减少分配/构造开销，热路径更可预测。
- **NRVO 仍需手写**：返回有名变量时 NRVO 仍可选，HFT 热路径要测编译器是否真的省略（`-O2` 通常会）。
- **配合 `return {...}`**：`return Tick{ts, px, qty};` 是 prvalue，强制省略。

## 自测题

1. C++17 的强制拷贝省略和 C++14 的 RVO 有什么本质区别？
2. 不可拷贝不可移动的类型在 C++17 为什么能作为返回值？
3. 强制省略只对 prvalue 生效，对 lvalue 返回呢？
4. `foo(std::string("hi"))` 在 C++17 里发生几次构造？
5. HFT 返回行情对象为什么用 `return Tick{...}` 而不是 `Tick t; ...; return t;`？

## 代码自测

### Q1: 保证的拷贝省略
```cpp
struct Widget {
    Widget() { std::puts("default"); }
    Widget(const Widget&) { std::puts("copy"); }
    Widget(Widget&&) { std::puts("move"); }
};

Widget make() { return Widget(); }  // 返回临时对象

Widget w = make();  // C++17 前后分别输出什么？
```
> C++17 的保证拷贝省略与 C++14 的 NRVO 有什么区别？

<details>
<summary>答案与复习指引</summary>

**C++17**：输出 **"default"**（仅一次构造，无拷贝无移动）。保证的拷贝省略（guaranteed copy elision）是**语言规则**——编译器必须省略拷贝/移动，不是优化选项。

**C++14**：通常也输出 "default"（NRVO/RVO 优化），但标准允许拷贝/移动。如果关闭优化或编译器不优化，可能输出 "default" + "move"。

**区别**：
- C++14 的 RVO 是**优化**（编译器可以选择做）
- C++17 的保证省略是**语义**（编译器必须做，不可省略的对象直接在目标位置构造）

**关键场景**：不可移动不可拷贝的类型（如 `std::mutex`、`std::atomic`）在 C++17 可以从函数返回：
```cpp
std::mutex make_mutex() { return std::mutex(); }  // C++17 OK
```

**复习：** → [保证的拷贝省略](./README.md)
</details>
