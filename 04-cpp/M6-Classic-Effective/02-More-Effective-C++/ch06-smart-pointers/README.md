# 第六部分 智能指针与高级编程技法（Smart Pointers & More）

现代 C++ 资源管理进阶，理解智能指针底层原理。

## 条款

- [条款 28：手写基础版智能指针，理解智能指针的核心逻辑](./item28-手写基础版智能指针，理解智能指针的核心逻辑.md)
- [条款 29：实现引用计数智能指针（shared_ptr 前身），循环引用问题来源就在这一条](./item29-实现引用计数智能指针（shared_ptr前身），循环引用问题来源就在这一条.md)
- [条款 30：代理类（Proxy Class）设计模式，解决运算符重载、容器下标等语法痛点](./item30-代理类（ProxyClass）设计模式，解决运算符重载、容器下标等语法痛点.md)
- [条款 31：多重分派：让虚函数可以根据两个以上对象的类型动态匹配](./item31-多重分派：让虚函数可以根据两个以上对象的类型动态匹配.md)


## 章节摘要

智能指针进阶：手写智能指针、引用计数实现（`shared_ptr` 前身）、代理类模式、多重分派。

## 代码自测

### Q1: 引用计数原理

```cpp
// 简化的引用计数智能指针
template<typename T>
class CountedPtr {
    T *ptr;
    size_t *count;
public:
    CountedPtr(T *p) : ptr(p), count(new size_t(1)) {}
    CountedPtr(const CountedPtr& o) : ptr(o.ptr), count(o.count) { ++*count; }
    ~CountedPtr() { if (--*count == 0) { delete ptr; delete count; } }
};
```

> 拷贝和析构分别做什么？引用计数什么时候释放对象？

<details>
<summary>答案与复习指引</summary>

**拷贝：** 共享同一 `ptr` 和 `count`，`++*count`（引用计数 +1）
**析构：** `--*count`（引用计数 -1），如果归 0 则 `delete ptr`（释放对象）和 `delete count`（释放计数器）

**释放时机：** 最后一个 `CountedPtr` 析构时（引用计数归 0）。

**环引用问题：** A↔B 互引，计数永远不归 0 → 泄漏。`weak_ptr` 不增加强引用计数来打破环。

**线程安全：** `++*count` 和 `--*count` 需要原子操作才能线程安全（`shared_ptr` 用原子操作，本简化版没有）。

**复习：** → [条款 29：实现引用计数智能指针](./item29-实现引用计数智能指针（shared_ptr前身），循环引用问题来源就在这一条.md)
</details>

### Q2: 代理类模式

```cpp
// vector<bool> 的代理对象
std::vector<bool> v = {true, false, true};
auto& ref = v[0];  // ref 的类型是什么？
ref = false;       // 修改 v[0] 吗？
```

> `ref` 的类型是什么？为什么不能用 `auto&`？

<details>
<summary>答案与复习指引</summary>

**`ref` 的类型是 `std::vector<bool>::reference`（代理对象），不是 `bool&`。** `vector<bool>` 用位压缩存储，没有真正的 `bool` 对象可以引用——`operator[]` 返回代理对象，它内部持有指向字节的指针+位掩码。

**`ref = false` 会修改 `v[0]`**——代理对象的 `operator=` 写回对应的位。

**风险：** 如果 `v` 被销毁/扩容，`ref` 成为悬垂代理 → UB。

**教训：** 代理类型看起来像值但实际是引用/句柄。`auto` 会忠实地绑定代理类型——需要 `static_cast<bool>(v[0])` 转成真正的值。

**复习：** → [条款 30：代理类设计模式](./item30-代理类（ProxyClass）设计模式，解决运算符重载、容器下标等语法痛点.md)
</details>
