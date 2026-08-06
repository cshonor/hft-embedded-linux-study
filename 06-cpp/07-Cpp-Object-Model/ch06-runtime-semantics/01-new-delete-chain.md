# 6.1 new/delete 完整链路

> 第 6 章 运行时语义 · 上一节：[本章导读](README.md) · 下一节：[6.2 RTTI](02-rtti.md)

## 这节讲什么

`new[]` 在头部存元素数（额外开销），`delete[]` 据此逐个析构。`operator new` 可重载接 mempool。

---

## 数组 new[]

```cpp
Widget* arr = new Widget[10];
// 内存布局：[count (8B)] [Widget 0] [Widget 1] ... [Widget 9]
// 头部存元素数，delete[] 据此逐个析构

delete[] arr;  // 正确：逐个析构 + 释放
delete arr;    // UB！只析构第一个，内存释放方式也可能错
```

`delete` 误配 `delete[]` 是 UB——`delete` 不知道头部有元素数，可能只析构第一个或释放错地址。

### operator new 重载

```cpp
void* operator new(size_t n) { return my_pool.alloc(n); }
void operator delete(void* p) { my_pool.free(p); }
```

全局/类级重载，接 mempool/hugepage。

---

## 新手要点

- **`new[]` 配 `delete[]`**：数组用 `new[]` 分配就用 `delete[]` 释放。不配对是 UB。
- **现代 C++ 不用裸 new[]/delete[]**：用 `std::vector` 或 `std::unique_ptr<T[]>` 替代，自动管理。

---

## HFT 关联

- **`operator new` 重载接 mempool**：HFT 重载 `operator new` 接预分配 mempool，零系统 `malloc`。

---

## 自测题

1. `new T[N]` 的内存布局是什么？头部存什么？
2. 为什么 `delete` 不能配 `new[]`？
3. `operator new` 如何重载？HFT 为什么要重载它？

---

## 参考与延伸

- 下一节：[6.2 RTTI](02-rtti.md)
- 回到：[第 6 章 运行时语义](README.md)
