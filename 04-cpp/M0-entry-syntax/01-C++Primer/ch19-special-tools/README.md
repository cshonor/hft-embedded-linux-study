# 第 19 章 特殊工具与技术

全书最后一章，偏底层、极限场景、库开发者才高频使用，业务开发极少触碰。

## 小节

- [19.1 内存分配与 new 重载](./19.1-内存分配与new重载.md)
- [19.2 运行时类型信息补充 & 枚举进阶](./19.2-运行时类型信息补充&枚举进阶.md)
- [19.3 类成员指针](./19.3-类成员指针.md)
- [19.4 嵌套类](./19.4-嵌套类.md)
- [19.5 联合（union）与位域](./19.5-联合（union）与位域.md)
- [19.6 可变参数模板（C++11）](./19.6-可变参数模板（C++11）.md)
- [19.7 转发引用与完美转发](./19.7-转发引用与完美转发.md)
- [19.8 其他底层工具](./19.8-其他底层工具.md)
- [学习优先级](./19.9-学习优先级.md)


## 章节摘要

特殊工具与技术：内存分配与 `new` 重载、类成员指针、嵌套类、联合（`union`）与位域、可变参数模板（深入）、转发引用与完美转发（深入）。

### 和 C 的区别

| C | C++ |
|---|-----|
| `malloc`/`free` | `operator new`/`operator delete`（可重载） |
| 无成员指针 | `int Class::*p`（指向成员的指针） |
| `union` 无限制 | `union` 限制（不能有构造/析构的成员） |
| `__VA_ARGS__` | 可变参数模板（类型安全） |
| 无完美转发 | `std::forward`（保留左右值性） |

## 章节自测

### Q1: operator new 重载

```cpp
class FastAlloc {
    void *operator new(size_t n) {
        // 从内存池分配，不走 malloc
        return pool_alloc(n);
    }
    void operator delete(void *p) {
        pool_free(p);
    }
};
FastAlloc *obj = new FastAlloc;  // 调用谁的 new？
delete obj;                       // 调用谁的 delete？
```

> `new FastAlloc` 的两步分别调用什么？类级 `operator new` 有什么用？

<details>
<summary>答案与复习指引</summary>

**两步：**
1. `FastAlloc::operator new(sizeof(FastAlloc))` — 从内存池分配（不走全局 `malloc`）
2. `FastAlloc` 的构造函数 — 在分配的内存上构造对象

**类级 `operator new` 用途：**
- 自定义内存池（避免 `malloc` 锁竞争 + 碎片）
- 统计内存使用
- HFT 对象池：预分配 + 回收，零运行时分配

**`delete` 同理：** 先调析构，再调 `FastAlloc::operator delete` 释放回池。

**复习：** → [19.1 内存分配与 new 重载](./19.1-内存分配与new重载.md)
</details>

### Q2: 成员指针

```cpp
struct Point { int x, y; };
int Point::*px = &Point::x;  // 指向成员的指针
Point p{10, 20};
Point *pp = &p;
std::cout << p.*px << " " << pp->*px;
```

> 输出是什么？成员指针和普通指针有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `10 10`

**成员指针 vs 普通指针：**
- 普通指针指向具体内存地址
- 成员指针是**偏移量**——记录"成员在对象内的位置偏移"
- 使用时需要结合具体对象：`p.*px` 或 `pp->*px`
- 成员函数指针更复杂（可能含 vtable 偏移 + this 调整信息），大小通常是 2 个指针

**C 没有成员指针**——C 的 `struct` 没有成员函数，不需要这种机制。

**复习：** → [19.3 类成员指针](./19.3-类成员指针.md)
</details>

### Q3: union 限制

```cpp
union Value {
    int i;
    std::string s;  // C++11: 合法吗？
    Value() : s() {}
    ~Value() {}
};
```

> C++11 中 `union` 含 `std::string` 成员合法吗？有什么限制？

<details>
<summary>答案与复习指引</summary>

**C++11 起合法**，但有限制：
1. 含非平凡成员（有构造/析构/拷贝）的 `union`，必须手动管理哪个成员是活跃的
2. 不能同时使用多个成员——写入一个成员后读取另一个是 UB
3. `union` 需要自定义构造/析构来管理非平凡成员的生命周期

**C 的 `union`：** 只能含 POD 类型（无构造/析构），简单按位覆盖。

**典型用途：** `std::variant`/`std::any` 的底层实现用 tagged union——外加一个 tag 标记当前活跃成员。

**复习：** → [19.5 联合（union）与位域](./19.5-联合（union）与位域.md)
</details>

### Q4: 完美转发深入

```cpp
template<typename T, typename... Args>
auto make_unique(Args&&... args) -> std::unique_ptr<T> {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
// 调用 make_unique<Widget>(42, "hello", 3.14)
// args 的类型分别是什么？
```

> `Args&&...` 是什么？`std::forward<Args>(args)...` 做了什么？

<details>
<summary>答案与复习指引</summary>

**`Args&&...`：** 万能引用参数包——每个参数都能接受左值或右值。

**`std::forward<Args>(args)...：** 逐个参数完美转发——保留每个参数的原始左右值性。如果原始参数是左值就转发为左值引用，是右值就转发为右值引用。

**这是 `std::make_unique` 的核心**——它把构造参数原封不动地转发给 `T` 的构造函数，不丢失任何信息（左右值性、const 性）。如果不用 `forward`，所有参数都会变成左值，无法触发移动构造。

**复习：** → [19.7 转发引用与完美转发](./19.7-转发引用与完美转发.md)
</details>
