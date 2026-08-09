# Item 8：优先 nullptr 而非 0 和 NULL

> 第 3 章 移步现代 C++ · Item 8 · 上一节：[Item 7 () vs {}](item07-parens-vs-braces.md)

## 为什么要学这个（先建立直觉）

C 里空指针怎么写？两种方式：

```c
int* p = NULL;    // 方式一
int* p = 0;       // 方式二
// NULL 通常定义为 ((void*)0) 或 0
```

在 C 里这基本没问题——C 没有函数重载，`NULL` 传给函数就是空指针。但 C++ 有函数重载：

```cpp
void f(int);       // 重载 1
void f(char*);     // 重载 2

f(0);       // 调 f(int)——你以为传空指针，实际传了整数 0
f(NULL);    // 可能调 f(int)——NULL 的类型依赖实现，可能是 int
f(nullptr); // 调 f(char*)——这才是你想要的
```

**根因：** C++ 的 `NULL` 不是指针类型，它是整型（`0` 或 `0L`）。`nullptr` 是 C++11 引入的真正的空指针常量，类型是 `std::nullptr_t`，能隐式转任意指针但不能转整型。

---

## 这节讲什么

`0` 和 `NULL` 都是整型字面量，在指针与整型重载时会误选整型。`nullptr` 的类型是 `std::nullptr_t`，能隐式转任意指针但不能转整型——彻底消除歧义。

---

## 核心机制

### 重载歧义消除

```cpp
void f(int);
void f(Widget*);
f(0);          // 调 f(int)！不是 f(Widget*)
f(NULL);       // 仍可能调 f(int)（NULL 的类型依赖实现）
f(nullptr);    // 调 f(Widget*)，正确
```

### nullptr 在模板中的优势

```cpp
// 模板推导：传 0 会被推成 int
template<class T> void g(T x);
g(0);          // T = int —— 不是指针！
g(nullptr);    // T = nullptr_t —— 正确，后续可隐式转任意指针

// 实际场景：锁的默认参数
template<class Mutex>
class lock_guard {
public:
    // 如果想传"空锁"做默认参数
    lock_guard(Mutex* m = nullptr);  // OK，nullptr 正确推导
};
```

### nullptr 的比较行为

```cpp
int* p = nullptr;
if (p == nullptr) { /* true */ }
if (p) { /* false */ }
if (!p) { /* true */ }

// nullptr 可以和任意指针比较
char* cp = nullptr;
Widget* wp = nullptr;
// cp == wp;  // 编译失败——不同指针类型不能比较
// cp == nullptr;  // OK
// wp == nullptr;  // OK
```

`nullptr` 的三大优势：
1. **不会误选整型重载**
2. **模板推导正确**：`g(nullptr)` 推出 `T = nullptr_t`，而非 `int`
3. **代码意图清晰**：`nullptr` 一眼看出是空指针，`0` 需要看上下文

---

## 常见错误（新手踩坑）

**错误 1：在重载函数中传 0 想表达空指针**
```cpp
void process(int x);
void process(Widget* w);
process(0);  // 调 process(int)，不是 process(Widget*)！
```
**修正：** 用 `process(nullptr);`。

**错误 2：模板中传 NULL 导致类型推导错误**
```cpp
template<class T> void set_callback(T cb) { /* ... */ }
set_callback(NULL);  // T = int 或 long，不是函数指针！
set_callback(nullptr);  // T = nullptr_t
```
**修正：** 模板场景一律用 `nullptr`。

**错误 3：混用 NULL 和 nullptr 导致代码风格不一致**
```cpp
// 老代码
if (ptr == NULL) { ... }
// 新代码
if (ptr == nullptr) { ... }
```
**修正：** 统一用 `nullptr`。`NULL` 在 C++ 里没有任何 `nullptr` 不具备的优势。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 空指针写法 | `NULL` 或 `0` | `nullptr` | C++ 有重载，`NULL`/`0` 是整型会误选 |
| `NULL` 的定义 | `((void*)0)`（指针类型） | `0` 或 `0L`（整型！） | C++ 不允许 `void*` 隐式转其他指针 |
| 模板推导 | 不适用（C 无模板） | `nullptr` → `T = nullptr_t` | 正确表达"空指针"意图 |
| 类型安全 | 无重载所以无所谓 | `nullptr` 有独立类型 | 消除重载歧义 |

**一句话总结：** C 程序员记住——C++ 的 `NULL` 不是 C 的 `NULL`（C 的 `NULL` 是指针，C++ 的是整型），C++ 里空指针一律用 `nullptr`。

---

## HFT 关联

- **模板推导正确性**：策略工厂 `template<class T> T* create(Args...)` 里传 `nullptr` 做默认参数时，类型推导正确；传 `0` 会被推成 `int`。
- **回调注册**：`set_handler(nullptr)` 清除回调比 `set_handler(NULL)` 更安全——`NULL` 在模板中可能被推成整型，导致编译错误或运行时 bug。
- **指针比较清晰**：`if (order_ptr == nullptr)` 比 `if (!order_ptr)` 更明确地表达"检查空指针"意图，代码审查时一目了然。

---

## 自测题

1. `f(0)` 在 `void f(int); void f(Widget*);` 重载集里调用哪个？`f(nullptr)` 呢？
2. `nullptr` 的类型是什么？它能隐式转成 `int` 吗？
3. 为什么 `NULL` 在 C++ 里不安全？C 的 `NULL` 和 C++ 的 `NULL` 定义有什么不同？
4. `template<class T> void g(T x); g(nullptr);` 推出 `T` 是什么？`g(0)` 呢？
5. 下面代码有什么问题？
```cpp
void register_cb(int id);
void register_cb(void(*cb)());
register_cb(0);       // A
register_cb(nullptr); // B
```

---

## 参考与延伸

- 下一节：[Item 9 using 别名](item09-using.md)
- 回到：[第 3 章 移步现代 C++](README.md)
