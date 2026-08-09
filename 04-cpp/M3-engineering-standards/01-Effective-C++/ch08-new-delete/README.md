# 第八章 定制 new 与 delete

共 5 条条款。

## 条款

- [条款 48：了解 new、delete 的各种含义](./item48-了解new、delete的各种含义.md)
- [条款 49：重载 operator new、operator delete 遵守常规规则](./item49-重载operatornew、operatordelete遵守常规规则.md)
- [条款 50：写了 placement new，一定要配套写 placement delete](./item50-写了placementnew，一定要配套写placementdelete.md)
- [条款 51：重载全局内存分配函数三思而后行](./item51-重载全局内存分配函数三思而后行.md)
- [条款 52：遵守 placement new/delete 完整配对规则](./item52-遵守placementnewdelete完整配对规则.md)


## 章节摘要

定制 new/delete：理解各种 new/delete 含义、重载规则、placement new/delete 配对、谨慎重载全局 operator new。

## 代码自测

### Q1: new 的两步

```cpp
Widget *p = new Widget;
// 这一行做了几件事？
delete p;
// 这一行做了几件事？
```

> `new Widget` 和 `delete p` 分别做了几步？

<details>
<summary>答案与复习指引</summary>

**`new Widget` 两步：**
1. `operator new(sizeof(Widget))` — 分配内存（默认走 `malloc`）
2. `Widget` 构造函数 — 在分配的内存上构造对象

**`delete p` 两步：**
1. `Widget` 析构函数 — 析构对象
2. `operator delete(p)` — 释放内存（默认走 `free`）

**重载 `operator new` 只改变第一步**——内存分配方式，不影响构造/析构。

**`placement new`：** `new (buf) Widget` — 跳过第一步（用已有内存 `buf`），只执行第二步（构造）。

**复习：** → [条款 48：了解 new、delete 的各种含义](./item48-了解new、delete的各种含义.md)
</details>

### Q2: placement new 配对

```cpp
char buf[sizeof(Widget)];
Widget *p = new (buf) Widget;  // placement new
// ... 使用 p ...
// p 的析构和内存释放怎么做？
```

> 如何正确清理 placement new 创建的对象？

<details>
<summary>答案与复习指引</summary>

**必须显式调用析构 + 不调 `delete`：**
```cpp
p->~Widget();  // 显式析构
// 不调 delete p！—— buf 是栈数组，不是 new 分配的
```

**原因：** `placement new` 没有分配内存（用的是已有 `buf`），所以 `delete` 会试图释放非堆内存 → UB。只需显式调析构函数清理对象。

**用途：** 内存池/对象池——预分配大块内存，用 `placement new` 在上面反复构造对象，避免频繁 `malloc`。

**STL 容器底层：** `vector` 扩容时用 `placement new` 在新内存上移动构造元素。

**复习：** → [条款 50：写了 placement new，一定要配套写 placement delete](./item50-写了placementnew，一定要配套写placementdelete.md)
</details>
