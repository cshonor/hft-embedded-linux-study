# 6.1 new/delete 完整链路

> 第 6 章 运行时语义 · 上一节：[本章导读](README.md) · 下一节：[6.2 RTTI](02-rtti.md)

## 这节讲什么

`new[]` 在头部存元素数（额外开销），`delete[]` 据此逐个析构。`operator new` 可重载接 mempool。理解完整链路才能正确重载和优化。

---

## 为什么要学这个（先建立直觉）

C 程序员的数组分配很直接：

```c
// C：malloc 数组
int* arr = malloc(10 * sizeof(int));  // 纯内存分配
free(arr);  // 纯内存释放
// 没有构造/析构，没有额外开销
```

C++ 的 `new[]` 更复杂——它需要记录元素数量，以便 `delete[]` 知道析构几个：

```cpp
Widget* arr = new Widget[10];
// 内存布局：[元素数(8B)] [Widget 0] [Widget 1] ... [Widget 9]
// 头部 8 字节存元素数（cookie），delete[] 据此逐个析构
delete[] arr;  // 正确：读 cookie → 逐个析构 → 释放
delete arr;    // UB！不知道有几个元素，只析构第一个
```

---

## 完整链路详解

### new[] 的展开

```cpp
Widget* arr = new Widget[10];
// 1. operator new[](sizeof(Widget) * 10 + cookie_size)  // 分配内存 + cookie
//    cookie_size 通常 = 8（存元素数）
// 2. 逐个 placement new 构造：new(mem + i) Widget()
// 内存布局：[count=10 (8B)] [Widget 0] [Widget 1] ... [Widget 9]
```

### delete[] 的展开

```cpp
delete[] arr;
// 1. 读 cookie 获取元素数（10）
// 2. 从最后一个开始逐个析构：arr[9].~Widget(), arr[8].~Widget(), ...
// 3. operator delete[](arr)  // 释放内存（含 cookie）
```

### new/delete vs new[]/delete[]

```cpp
Widget* w = new Widget();     // 单个对象，无 cookie
delete w;                      // 正确
Widget* arr = new Widget[10]; // 数组，有 cookie
delete[] arr;                  // 正确
// delete arr;  // UB！把数组当单个对象析构
```

### operator new 重载

```cpp
// 全局重载——影响所有 new
void* operator new(size_t n) {
    return my_pool.alloc(n);
}
void operator delete(void* p) {
    my_pool.free(p);
}

// 类级重载——只影响该类
class FastObject {
public:
    void* operator new(size_t n) { return pool.alloc(n); }
    void operator delete(void* p) { pool.free(p); }
};
FastObject* o = new FastObject();  // 用类级 operator new
```

---

## 常见错误（新手踩坑）

### 错误 1：new[] 配 delete

```cpp
Widget* arr = new Widget[10];
delete arr;  // UB！只析构第一个，其余 9 个不析构
// cookie 也没被正确读取
```

### 错误 2：忘了数组 cookie 的开销

```cpp
// 预分配内存 + placement new[]
char buf[sizeof(Widget) * 10 + 8];  // 需要 +8 给 cookie
Widget* arr = new (buf) Widget[10]; // placement new[]
// 但 placement new[] 的 cookie 处理因编译器而异——避免手动做
```

### 错误 3：重载 operator new 忘了对应 delete

```cpp
class FastObject {
public:
    void* operator new(size_t n) { return pool.alloc(n); }
    // 忘了 operator delete → 用全局 delete 释放 pool 分配的内存 → UB
};
// 修正：void operator delete(void* p) { pool.free(p); }
```

---

## 和 C 的区别

| 特性 | C malloc/free | C++ new/delete |
|------|-------------|----------------|
| 数组分配 | `malloc(n * sizeof(T))` | `new T[n]`（含 cookie） |
| 构造/析构 | 无 | 自动逐个调 |
| 配对错误 | 无（free 就是 free） | **UB**（new[]/delete 不配对） |
| 自定义分配 | N/A | 重载 operator new |
| cookie 开销 | 无 | new[] 有 8B cookie |

---

## HFT 关联

1. **operator new 重载接 mempool**：HFT 重载 `operator new` 接预分配 mempool，零系统 malloc。
2. **避免 new[]/delete[]**：用 `std::vector` 替代裸 `new[]`——自动管理构造/析构/扩容。
3. **placement new + 手动管理**：HFT 对象池用 `new(buf) Widget()` 在预分配内存上构造，零 malloc，确定性延迟。

---

## 代码自测

### Q1: 内存布局

```cpp
class Widget { int x; public: Widget() {} ~Widget() {} };
Widget* arr = new Widget[5];
// arr 指向的内存布局是什么？arr[0] 在哪？
```

<details>
<summary>答案与复习指引</summary>

布局：`[count=5 (8B)] [Widget 0] [Widget 1] ... [Widget 4]`。`arr` 指向 Widget 0 的位置（cookie 在 arr 之前）。`arr - 8` 处存着元素数 5。`delete[]` 读取这个 cookie 知道要析构 5 个。

**复习：** → [6.1 new/delete 完整链路](./01-new-delete-chain.md)
</details>

### Q2: 配对错误

```cpp
Widget* a = new Widget();       // A
Widget* b = new Widget[10];     // B
delete a;     // 正确吗？
delete b;     // 正确吗？
delete[] a;   // 正确吗？
delete[] b;   // 正确吗？
```

<details>
<summary>答案与复习指引</summary>

`delete a`：正确。`delete b`：**UB**（new[] 配 delete）。`delete[] a`：**UB**（new 配 delete[]）。`delete[] b`：正确。规则：`new` 配 `delete`，`new[]` 配 `delete[]`，严格配对。

**复习：** → [6.1 new/delete 完整链路](./01-new-delete-chain.md)
</details>

### Q3: operator new 重载

```cpp
class FastAlloc {
    static Pool pool;
public:
    void* operator new(size_t n) { return pool.alloc(n); }
    void operator delete(void* p) { pool.free(p); }
};
FastAlloc* obj = new FastAlloc();
delete obj;
// 分配和释放经过什么路径？
```

<details>
<summary>答案与复习指引</summary>

`new FastAlloc()`：调 `FastAlloc::operator new` → `pool.alloc()` → placement new 构造。`delete obj`：调 `~FastAlloc()` → `FastAlloc::operator delete` → `pool.free()`。全程不经过系统 malloc/free——零系统调用，确定性延迟。HFT 惯用法。

**复习：** → [6.1 new/delete 完整链路](./01-new-delete-chain.md)
</details>

### Q4: vector 替代

```cpp
// 方案 A：裸 new[]
Widget* arr = new Widget[100];
// ... 使用 ...
delete[] arr;

// 方案 B：vector
std::vector<Widget> vec(100);
// 哪个更好？为什么？
```

<details>
<summary>答案与复习指引</summary>

方案 B（vector）。优势：①自动管理构造/析构/扩容；②不会 new[]/delete[] 不配对；③可动态改变大小；④RAII 保证不泄漏。裸 new[] 的唯一优势是无额外开销（vector 有 capacity 管理），但 HFT 中用 `vector::reserve()` 预分配后两者性能相当。

**复习：** → [6.1 new/delete 完整链路](./01-new-delete-chain.md)
</details>

---

## 参考与延伸

- 下一节：[6.2 RTTI](02-rtti.md)
- 回到：[第 6 章 运行时语义](README.md)
