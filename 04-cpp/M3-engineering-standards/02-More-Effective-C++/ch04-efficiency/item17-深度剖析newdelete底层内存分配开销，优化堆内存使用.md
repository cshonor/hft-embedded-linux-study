# 条款 17：深度剖析 new/delete 底层内存分配开销，优化堆内存使用

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
// new 涉及堆分配与簿记开销
std::vector<int> v;  // 连续内存，少次 new
v.reserve(1000);
```

---

## 代码自测

**题目 1：** 以下两种内存分配方式有什么性能差异？
```cpp
// 方式A：逐个 new
for (int i = 0; i < 1000; i++)
    arr[i] = new Widget;
// 方式B：预分配内存池
Widget* pool = (Widget*)malloc(sizeof(Widget) * 1000);
for (int i = 0; i < 1000; i++)
    new(&pool[i]) Widget;
```

<details>
<summary>参考答案</summary>

方式A：每次 `new` 调用 `malloc`——涉及系统调用、堆碎片管理、锁竞争，1000 次调用开销大。方式B：一次 `malloc` 分配连续内存，然后用 placement new 在预分配内存上构造——只有 1 次系统调用，内存连续（缓存友好）。HFT 场景常用内存池：预分配大块内存，运行时只做指针偏移，无系统调用。

</details>
