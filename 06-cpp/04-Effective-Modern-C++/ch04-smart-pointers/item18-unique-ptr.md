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

---

## 参考与延伸

- 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
