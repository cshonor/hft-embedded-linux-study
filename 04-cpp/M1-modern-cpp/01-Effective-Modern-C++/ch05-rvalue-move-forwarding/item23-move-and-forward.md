# Item 23：理解 std::move 和 std::forward

> 第 5 章 右值引用、移动语义与完美转发 · Item 23 · 下一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 为什么要学这个（先建立直觉）

C 的赋值和传参都是拷贝——没有"移动"的概念：

```c
// C：拷贝一个 buffer 要 memcpy 整块数据
void process(Buffer* dst, Buffer* src) {
    memcpy(dst->data, src->data, src->size);  // O(n) 拷贝
    dst->size = src->size;
}

// C：想要"转移所有权"只能手动交换指针
void transfer(Buffer* dst, Buffer* src) {
    dst->data = src->data;   // 指针交接 O(1)
    dst->size = src->size;
    src->data = NULL;         // 原对象置空
    src->size = 0;
}
```

C++ 的移动语义把"指针交接"模式语言化了——编译器自动调用移动构造函数，你只需用 `std::move` 表示"我允许你搬走它"：

```cpp
std::vector<int> v1 = {1, 2, 3, 4, 5};
std::vector<int> v2 = std::move(v1);  // O(1) 指针交接，v1 变空
// v1 现在是有效但未指定状态（通常是空）
```

**关键认知：** `std::move` 本身不移动任何东西——它只是一个 `static_cast<T&&>`，告诉编译器"这个值可以被移动"。真正的移动发生在移动构造函数里。

---

## 这节讲什么

`std::move` 不移动任何东西——它是无条件的右值转换。`std::forward` 是有条件的右值转换。理解两者的区别是移动语义的基础。

---

## 核心区别

```cpp
std::move(x);         // x 无条件变右值——不管 x 是左值还是右值
std::forward<T>(x);   // 仅当 T 是非引用（右值）时才转右值——保留原始左右值性
```

| | `std::move` | `std::forward` |
|---|------------|----------------|
| 转换条件 | 无条件 | 有条件（看 T） |
| 用途 | "我要无条件搬走它" | "保留它的左右值性转发" |
| 本质 | `static_cast<T&&>` | `static_cast<T&&>`（条件触发） |
| 使用场景 | 右值引用参数、return local | 万能引用参数转发 |

### std::move 的实际效果

```cpp
std::string s1 = "hello";
std::string s2 = std::move(s1);  // s1 变右值 → 匹配 string 的移动构造
// s2 拿走了 s1 的内部指针，s1 变空（有效但未指定）

// 如果类型没有移动构造呢？
int x = 42;
int y = std::move(x);  // int 没有移动构造 → 退回拷贝 → y = 42, x 不变
```

### std::forward 的实际效果

```cpp
template<class T>
void wrapper(T&& x) {
    target(std::forward<T>(x));
}

std::string s = "hello";
wrapper(s);           // x 是左值 → forward 保留左值 → target(string&)
wrapper(std::string("temp"));  // x 是右值 → forward 转为右值 → target(string&&)

// 如果这里用 std::move 而非 std::forward：
template<class T>
void bad_wrapper(T&& x) {
    target(std::move(x));  // 总是转右值！左值 s 被掏空 → 悬垂！
}
```

---

## 常见错误（新手踩坑）

**错误 1：对万能引用用 std::move**
```cpp
template<class T>
void set_name(T&& name) {
    this->name = std::move(name);  // 如果 name 是左值 → 被掏空！
}
std::string s = "Alice";
obj.set_name(s);  // s 被掏空！
```
**修正：** `this->name = std::forward<T>(name);`——万能引用用 `forward`。

**错误 2：return std::move(local) 阻止 NRVO**
```cpp
Widget make_widget() {
    Widget w;
    // return std::move(w);  // 阻止 NRVO！编译器不能 RVO 了
    return w;                 // OK，NRVO 直接在返回值位置构造
}
```
**修正：** 直接 `return w;`，让编译器做 NRVO。

**错误 3：move 后还用原对象**
```cpp
std::string s = "hello";
auto s2 = std::move(s);
std::cout << s.size();  // UB？不——s 是有效但未指定状态
// 可能输出 0（通常），也可能是原值——不要依赖
```
**修正：** `move` 后不要读原对象的值。可以给它赋新值或让它析构。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 资源转移 | 手动交换指针 | `std::move` + 移动构造 | 语言级支持 |
| 拷贝代价 | 总是 O(n) | 移动 O(1)，拷贝 O(n) | 移动只交接指针 |
| "搬走"标记 | 手动置空 | `std::move` | 编译器自动调用移动构造 |
| 转发 | 不适用 | `std::forward` | 保留左右值性 |

**一句话总结：** C 程序员记住——`std::move` 是"我允许你搬走它"的标记，本身不移动。万能引用用 `forward`，右值引用用 `move`。`move` 后不要用原对象的值。

---

## HFT 关联

- **订单簿扩容**：`vector<Order>` 扩容时 `move` 让扩容从 O(n) 拷贝降到 O(n) 移动（每个元素 O(1) 指针交接）。
- **回调转发**：`template<class F> void onEvent(F&& cb) { storage.push_back(std::forward<F>(cb)); }` 避免不必要的拷贝。
- **返回值优化**：HFT 代码中返回大对象时不要 `return std::move(local)`——让 NRVO 生效。

---

## 自测题

1. `std::move` 实际做了什么？它本身会移动资源吗？
2. `std::move` 和 `std::forward` 的区别是什么？
3. `std::move(x)` 后 x 处于什么状态？
4. 如果类型没有移动构造，`std::move` 会怎样？
5. 下面代码有什么问题？
```cpp
template<class T>
void process(T&& x) {
    store(std::move(x));
}
std::string s = "data";
process(s);
std::cout << s;
```

---

## 参考与延伸

- 下一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
