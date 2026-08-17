# 第四部分 效率优化（Efficiency）

C++ 隐性开销、拷贝优化、临时对象、内存分配提速。

## 条款

- [条款 16：理清 80% 性能损耗的来源：临时对象的产生与消除手段](./item16-理清80%性能损耗的来源：临时对象的产生与消除手段.md)
- [条款 17：深度剖析 new/delete 底层内存分配开销，优化堆内存使用](./item17-深度剖析newdelete底层内存分配开销，优化堆内存使用.md)
- [条款 18：通过重载 operator new 实现自定义内存池，减少频繁堆分配损耗](./item18-通过重载operatornew实现自定义内存池，减少频繁堆分配损耗.md)
- [条款 19：理解临时对象、拷贝构造、返回值优化 RVO/NRVO](./item19-理解临时对象、拷贝构造、返回值优化RVONRVO.md)
- [条款 20：按需选用静态绑定/动态绑定（虚函数），不要无脑虚函数增加开销](./item20-按需选用静态绑定动态绑定（虚函数），不要无脑虚函数增加开销.md)


## 章节摘要

效率优化：临时对象消除、`new`/`delete` 底层开销、自定义内存池、RVO/NRVO、按需选虚函数。

## 代码自测

### Q1: 临时对象

```cpp
std::string s1 = "hello", s2 = "world";
std::string s3 = s1 + s2 + "!";  // 产生几个临时对象？
```

> `s1 + s2 + "!"` 产生几个临时 `string` 对象？

<details>
<summary>答案与复习指引</summary>

**至少 1 个临时对象**（`s1 + s2` 的结果），然后 `+ "!"` 产生第二个临时对象（或编译器优化合并）。

**优化：** `s3 = s1; s3 += s2; s3 += "!";` — 无临时对象，直接在 `s3` 上追加。

**C++11 移动语义减轻了影响：** 临时对象可以被移动而非拷贝，但仍有构造/析构开销。

**HFT 教训：** 热路径避免不必要的临时对象——用 `+=` 替代 `+`，用 `reserve` 预分配，用 `string_view` 避免构造。

**复习：** → [条款 16：理清临时对象的产生与消除](./item16-理清80%性能损耗的来源：临时对象的产生与消除手段.md)
</details>

### Q2: RVO/NRVO

```cpp
std::string make_greeting() {
    std::string s = "Hello";
    s += ", World!";
    return s;  // NRVO: 几次拷贝？
}
std::string g = make_greeting();  // 总共几次构造/拷贝？
```

> NRVO 如何消除拷贝？如果 `return std::move(s)` 会怎样？

<details>
<summary>答案与复习指引</summary>

**NRVO（Named Return Value Optimization）：** 编译器把 `s` 直接在调用者的 `g` 内存上构造——零拷贝。`g` 只有 1 次构造（就是 `s` 的构造）。

**`return std::move(s)` 的灾难：** 阻止 NRVO！`std::move(s)` 把 `s` 变成右值引用返回，编译器退回移动构造——虽然移动比拷贝快，但不如 NRVO 零拷贝。

**规则：** 返回局部变量时直接 `return s;`，不要 `std::move`。编译器会自动做 NRVO 或移动语义。

**C++17 强制拷贝省略：** 返回纯右值（`return Widget();`）时，C++17 保证零拷贝（不是优化，是标准要求）。

**复习：** → [条款 19：理解 RVO/NRVO](./item19-理解临时对象、拷贝构造、返回值优化RVONRVO.md)
</details>
