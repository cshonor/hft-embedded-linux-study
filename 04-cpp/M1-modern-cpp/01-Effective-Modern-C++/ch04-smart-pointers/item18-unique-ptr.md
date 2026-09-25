# Item 18：用 std::unique_ptr 管理独占资源

> 第 4 章 智能指针 · Item 18 · 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 这节讲什么

`unique_ptr` 是**零开销抽象**——大小 = 裸指针（默认），开销 = 裸指针。它表达"唯一所有权"，不可拷贝、只能移动。这是 Modern C++ 管理资源的首选工具。

---

## 核心用法

```cpp
std::unique_ptr<Widget> make() { return std::make_unique<Widget>(); }
// 离开作用域自动 delete，无需手写
```

### 自定义删除器

```cpp
std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("f.txt", "r"), &fclose);
// 离开作用域自动 fclose
```

**删除器类型是 `unique_ptr` 类型的一部分**——不同删除器 = 不同类型。

### 与 C API 互操作

```cpp
unique_ptr<Widget> up = ...;
raw_call(up.get());      // 取裸指针传给 C 接口（不转移所有权）
Widget* p = up.release(); // 放弃所有权，交还给 C 管理
```

---

## 新手要点（和 C 的区别）

- **C 用 malloc/free + 手动管理**：C 程序员习惯 `Widget* p = malloc(...); ... free(p);`，忘了 free 就泄漏。C++ 的 `unique_ptr` 自动释放——RAII（资源获取即初始化）。
- **零开销**：`unique_ptr` 和裸指针一样大（1 个指针），析构时内联调用 `delete`——没有运行时额外开销。
- **不可拷贝**：`unique_ptr` 只能 `std::move`，不能 `=`。这强制了"唯一所有权"的语义。

---

## HFT 关联

- **管 mbuf / fd**：自定义删除器让 `unique_ptr` 管 DPDK `rte_mbuf*`（删除器调 `rte_pktmbuf_free`）或 POSIX `fd`（删除器调 `close`）。RAII 保证异常路径也不泄漏。
- **替代 C 的 goto cleanup**：C 用 `goto cleanup` 管理资源释放，C++ 用 `unique_ptr` 更安全且零开销。

---

## 自测题

1. `unique_ptr` 的大小是多少？为什么说是"零开销"？
2. 自定义删除器对 `unique_ptr` 的类型有什么影响？
3. `.get()` 和 `.release()` 有什么区别？
4. 为什么 `unique_ptr` 不可拷贝但可移动？

<details>
<summary>参考答案</summary>

1. 无自定义删除器（默认 `delete`）时，`std::unique_ptr<T>` 的大小通常**等于一个裸指针**（64 位平台 8 字节），运行时开销也为零：`*`/`->` 就是指针解引用，析构就是一次 `delete`。说它"零开销"是指与手写 `new`/`delete` 相比，它不引入额外的内存或时间成本——独占所有权完全靠**类型系统**（删除拷贝）在编译期表达，不需要引用计数，也不需要原子操作。

2. 删除器类型是 `unique_ptr` 类型的一部分：`unique_ptr<T, D>` 有两个模板参数。因此不同删除器的 `unique_ptr` 是**不同类型**，不能互相赋值或放入同一容器。另一个影响是大小：无状态删除器（空类、捕获为空的 lambda）通过空基类优化/压缩不会增加体积，`unique_ptr` 仍是一个指针大小；而有状态的删除器（如捕获了变量的函数对象、函数指针）会使对象变大（函数指针删除器通常是两个指针大小，即 16 字节）。

3. `.get()` 返回所管理的裸指针，**不改变所有权**，对象仍由 `unique_ptr` 负责释放，适合传给只需要观察/借用指针的接口。`.release()` **放弃所有权**：返回裸指针并把 `unique_ptr` 置空，此后释放责任转移给调用者（必须自己 `delete` 或交给别的 RAII 对象，否则泄漏）。另还有 `.reset(p)`：先释放当前对象再接管 `p`。

4. 因为它的语义是**独占所有权**（exclusive ownership）：同一时刻只能有一个 `unique_ptr` 拥有该对象，这样析构时才能确定地释放一次。拷贝会立刻产生两个拥有同一对象的智能指针，导致双重释放，所以拷贝构造与拷贝赋值被显式 `delete`（在编译期就禁止）。移动是安全的：源对象把指针移交后自身置空，所有权发生**转移**而非共享，因此 `std::move` 可用——这也是它能作为容器元素、作为工厂函数返回值的原因。如需共享所有权应改用 `shared_ptr`。

</details>

---

## 参考与延伸

- 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
