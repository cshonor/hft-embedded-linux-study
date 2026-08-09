# 5.3 new / delete 的两步

> 第 5 章 · 上一节：[5.2 存储期与生命周期](02-storage-duration.md) · 下一节：[5.4 异常安全](04-exception-safety.md)

## 这节讲什么

`new Widget` 不是一步——它先 `operator new` 分配内存，再调构造函数。`placement new` 在已分配内存上构造，省掉分配步骤。HFT 用 placement new + mempool 实现零 malloc。

---

## 为什么要学这个（先建立直觉）

C 程序员的 malloc/free 只管内存，不管初始化：

```c
// C：malloc 只分配内存
struct Widget_C* w = malloc(sizeof(struct Widget_C));
w->data = 0;  // 手动初始化
free(w);      // 只释放内存
```

C++ 的 new/delete 是两步操作——分配内存 + 调构造函数：

```cpp
Widget* w = new Widget();  // 1. operator new 分配内存  2. 调 Widget() 构造
delete w;                   // 1. 调 ~Widget() 析构      2. operator delete 释放
```

混用 malloc+delete 或 new+free 是未定义行为——malloc 不调构造，free 不调析构。

---

## 两步机制详解

### new 的展开

```cpp
Widget* p = new Widget(args);
// 编译器展开为：
// 1. void* mem = operator new(sizeof(Widget));  // 分配内存
// 2. p = new(mem) Widget(args);                  // placement new 构造
// 如果第 2 步抛异常，第 1 步分配的内存自动释放（operator delete）
```

### delete 的展开

```cpp
delete p;
// 编译器展开为：
// 1. p->~Widget();                    // 调析构函数
// 2. operator delete(p);              // 释放内存
```

### placement new

```cpp
char buf[sizeof(Widget)];
Widget* p = new (buf) Widget();  // 在 buf 上构造，不分配内存
// ... 使用 p ...
p->~Widget();                     // 手动析构（不 delete！）
// buf 本身由其他方式管理
```

### operator new 重载

```cpp
// 全局重载
void* operator new(size_t n) {
    return my_mempool.alloc(n);  // 接 mempool
}
void operator delete(void* p) {
    my_mempool.free(p);
}

// 类级重载
class FastAlloc {
public:
    void* operator new(size_t n) { return pool.alloc(n); }
    void operator delete(void* p) { pool.free(p); }
};
```

---

## 常见错误（新手踩坑）

### 错误 1：混用 malloc/delete

```cpp
Widget* w = (Widget*)malloc(sizeof(Widget));
// ... 使用 w ...
delete w;  // UB！malloc 没调构造函数，delete 调析构 → 析构未构造的对象
// 修正：用 new 替代 malloc
```

### 错误 2：placement new 忘了手动析构

```cpp
char buf[sizeof(Widget)];
Widget* p = new (buf) Widget();
// ... 使用 p ...
// 离开作用域忘了 p->~Widget() → 如果 Widget 有资源 → 泄漏
// placement new 构造的对象不能 delete（buf 不是 new 来的）
```

### 错误 3：new[] 配 delete（不配 []）

```cpp
Widget* arr = new Widget[10];
delete arr;    // UB！只析构第一个，其余 9 个不析构
delete[] arr;  // 正确：逐个析构 + 释放
```

---

## 和 C 的区别

| 特性 | C malloc/free | C++ new/delete |
|------|-------------|----------------|
| 内存分配 | malloc 只分配 | operator new 分配 |
| 初始化 | 手动 | 自动调构造函数 |
| 清理 | free 只释放 | 自动调析构函数 |
| 混用 | N/A | **UB**（malloc+delete / new+free） |
| 自定义分配 | N/A | 重载 operator new |
| placement | N/A | `new(buf) T()` 在已有内存上构造 |

---

## HFT 关联

1. **placement new + mempool**：`new(membuf) Widget` 在预分配 mempool 上构造，零 `malloc`——HFT 对象池惯用法。
2. **operator new 重载**：全局/类级重载 `operator new` 接 mempool/hugepage，避免系统 malloc 的不确定延迟。
3. **避免裸 new/delete**：用 `std::unique_ptr`/`make_unique` 替代裸 new——RAII 保证不泄漏。

---

## 代码自测

### Q1: 两步展开

```cpp
Widget* p = new Widget(42);
delete p;
// new 和 delete 各展开为几步？分别是什么？
```

<details>
<summary>答案与复习指引</summary>

new 展开为两步：①`operator new(sizeof(Widget))` 分配内存；②`placement new` 在该内存上调 `Widget(42)` 构造。
delete 展开为两步：①`p->~Widget()` 调析构函数；②`operator delete(p)` 释放内存。

**复习：** → [5.3 new/delete 的两步](./03-new-delete.md)
</details>

### Q2: placement new

```cpp
char buf[sizeof(Widget)];
Widget* p = new (buf) Widget();
// 如何正确销毁 p？
// 能 delete p 吗？
```

<details>
<summary>答案与复习指引</summary>

正确销毁：`p->~Widget()`（手动调析构）。不能 `delete p`——`buf` 不是 `operator new` 分配的，`delete` 会调 `operator delete(p)` 试图释放栈上的 `buf`，导致 UB。placement new 构造的对象只析构，不 delete。

**复习：** → [5.3 new/delete 的两步](./03-new-delete.md)
</details>

### Q3: 混用

```cpp
Widget* a = (Widget*)malloc(sizeof(Widget));
Widget* b = new Widget();
free(a);      // A
delete b;     // B
// 哪个安全？哪个 UB？
```

<details>
<summary>答案与复习指引</summary>

A：UB（malloc 没调构造，Widget 的析构可能操作未初始化的成员）。B：安全（new 调构造，delete 调析构，配对正确）。**malloc/free 和 new/delete 绝不混用。**

**复习：** → [5.3 new/delete 的两步](./03-new-delete.md)
</details>

### Q4: mempool

```cpp
// HFT 对象池惯用法
char pool_buf[sizeof(Order) * 10000];
int pool_idx = 0;
Order* createOrder() {
    Order* o = new (&pool_buf[pool_idx * sizeof(Order)]) Order();
    pool_idx++;
    return o;
}
// 这个方案有什么优势？
```

<details>
<summary>答案与复习指引</summary>

优势：①零系统 malloc（内存预分配在 pool_buf）；②确定性延迟（无 mmap/brk 系统调用）；③cache 友好（对象连续排列）。这是 HFT 对象池的标准做法——placement new + 预分配内存。注意需要手动析构和索引管理。

**复习：** → [5.3 new/delete 的两步](./03-new-delete.md)
</details>

---

## 参考与延伸

- 下一节：[5.4 异常安全](04-exception-safety.md)
- 回到：[第 5 章](README.md)
