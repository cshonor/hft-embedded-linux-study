# Item 7：容器销毁时删除指针

> 第 1 章 容器 · Item 7 · 上一节：[Item 6 警惕最烦人解析](item06-most-vexing-parse.md) · 下一节：[Item 8 不存 auto_ptr](item08-no-auto-ptr.md)

## 为什么要学这个（先建立直觉）

C 程序员管理指针数组的生命周期：

```c
Widget* widgets[100];
for (int i = 0; i < 100; ++i) widgets[i] = malloc(sizeof(Widget));
// ... 使用 ...
// 忘了释放 → 内存泄漏
for (int i = 0; i < 100; ++i) free(widgets[i]);  // 手动释放
```

C++ 的 `vector<Widget*>` 析构时**不会** `delete` 指针——它只释放存指针的数组本身，不关心指针指向的对象：

```cpp
{
    std::vector<Widget*> v;
    v.push_back(new Widget());
    v.push_back(new Widget());
}  // v 析构——但 Widget 对象没被 delete！内存泄漏！
```

---

## 这节讲什么

`vector<T*>`（或任何存裸指针的容器）析构时只释放指针数组本身，不 `delete` 指针指向的对象。必须手动 `delete` 或用智能指针容器让 RAII 自动释放。

---

## 泄漏演示

```cpp
{
    std::vector<Widget*> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(new Widget(i));
    // ... 使用 ...
}  // v 析构：释放了 100 个指针的数组，但 100 个 Widget 对象泄漏！
```

### 正确做法 1：手动 delete

```cpp
{
    std::vector<Widget*> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(new Widget(i));
    // ...
    for (auto p : v) delete p;  // 手动释放
    v.clear();  // 清空指针（不影响已 delete 的对象）
}
```

### 正确做法 2：智能指针容器（推荐）

```cpp
{
    std::vector<std::unique_ptr<Widget>> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(std::make_unique<Widget>(i));
    // ...
}  // v 析构 → unique_ptr 析构 → 每个 Widget 自动 delete ✅
```

### 正确做法 3：for_each + delete

```cpp
std::for_each(v.begin(), v.end(), [](Widget* p) { delete p; });
v.clear();
```

---

## 常见错误（新手踩坑）

### 错误 1：以为 vector<Widget*> 析构会 delete

```cpp
void f() {
    std::vector<Widget*> v;
    v.push_back(new Widget());
}  // 泄漏！vector 只释放存指针的内存，不 delete 对象
```

**修正：** 用 `vector<unique_ptr<Widget>>`。

### 错误 2：异常导致泄漏

```cpp
std::vector<Widget*> v;
v.push_back(new Widget());
v.push_back(new Widget());
process();  // 如果抛异常 → v 不析构（栈展开只析构局部变量）
// 等等，v 会析构——但只析构指针数组，Widget 对象仍泄漏
```

**修正：** 智能指针容器保证异常路径也安全。

### 错误 3：delete 后不清空指针

```cpp
for (auto p : v) delete p;
// v 里的指针现在指向已释放内存（悬空指针）
// 如果后续误用 v[i] → use-after-free
```

**修正：** `delete` 后立即 `v.clear()`，或直接用智能指针。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 指针数组释放 | 手动 `for + free` | 手动 `for + delete` 或智能指针 | RAII |
| 异常安全 | 无法保证 | 智能指针保证 | 栈展开调析构 |
| 生命周期管理 | 全手动 | `unique_ptr` 自动 | 零开销 RAII |

**一句话：** C 程序员习惯手动 `free` 指针数组。C++ 的 `vector<unique_ptr<T>>` 让容器析构自动 `delete` 每个对象——RAII 替代手动管理，异常安全且零开销。

---

## HFT 关联

- **策略对象池**：`vector<unique_ptr<Strategy>>` 管理策略对象生命周期，容器销毁时自动释放，无需手动 `delete`。
- **异常安全**：热路径如果抛异常（如限价单检查失败），智能指针容器保证栈展开时对象被正确释放。
- **替代 C 的 goto cleanup**：C 用 `goto cleanup` 管理资源释放，C++ 用 RAII 更安全。

---

## 代码自测

### Q1: 泄漏检测
```cpp
{
    std::vector<int*> v;
    for (int i = 0; i < 10; ++i) v.push_back(new int(i));
}  // 这里发生了什么？
```

<details>
<summary>答案</summary>

**内存泄漏**。vector 析构释放了存 10 个指针的数组内存（80 字节），但 10 个 `int` 对象（40 字节）没被 `delete`，永远泄漏。

**修正：** 用 `vector<unique_ptr<int>>` 或在析构前 `for (auto p : v) delete p;`。
</details>

### Q2: 智能指针容器
```cpp
#include <memory>
#include <vector>

struct Widget { int x; Widget(int v) : x(v) {} ~Widget() { puts("dtor"); } };

{
    std::vector<std::unique_ptr<Widget>> v;
    v.push_back(std::make_unique<Widget>(1));
    v.push_back(std::make_unique<Widget>(2));
}
```
> 离开作用域时输出什么？

<details>
<summary>答案</summary>

输出两次 "dtor"。vector 析构 → 每个 `unique_ptr` 析构 → `delete Widget` → 调用 `~Widget()`。

RAII 保证所有对象被正确释放，无需手动 `delete`。
</details>

### Q3: 异常安全
```cpp
std::vector<Widget*> v;
v.push_back(new Widget());
v.push_back(new Widget());
throw std::runtime_error("oops");  // 抛异常
// Widget 对象会泄漏吗？
```

<details>
<summary>答案</summary>

**会泄漏**。栈展开时 `v` 被析构（vector 析构），但 vector 只释放指针数组，不 `delete` 对象。两个 `Widget` 泄漏。

**修正：** 用 `vector<unique_ptr<Widget>>`——栈展开时 unique_ptr 析构自动 `delete`。
</details>

### Q4: 共享所有权
```cpp
// 多个容器需要共享同一组对象
std::vector<std::shared_ptr<Widget>> v1;
v1.push_back(std::make_shared<Widget>());
std::vector<std::shared_ptr<Widget>> v2;
v2.push_back(v1[0]);  // 两个容器共享同一个 Widget
// 当 v1 和 v2 都销毁后，Widget 会怎样？
```

<details>
<summary>答案</summary>

当最后一个 `shared_ptr` 被销毁时（v1 和 v2 都析构后），Widget 才被 `delete`。`shared_ptr` 用引用计数管理共享所有权——引用计数归零时释放对象。

`unique_ptr` 是独占所有权（不能拷贝），`shared_ptr` 是共享所有权（引用计数）。
</details>

---

## 参考与延伸

- 上一节：[Item 6 警惕最烦人解析](item06-most-vexing-parse.md)
- 下一节：[Item 8 不存 auto_ptr](item08-no-auto-ptr.md)
- 回到：[第 1 章 容器](README.md)
