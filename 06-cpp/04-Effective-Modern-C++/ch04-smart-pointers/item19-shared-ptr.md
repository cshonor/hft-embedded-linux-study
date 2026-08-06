# Item 19：用 std::shared_ptr 管理共享所有权

> 第 4 章 智能指针 · Item 19 · 上一节：[Item 18 unique_ptr](item18-unique-ptr.md)

## 这节讲什么

`shared_ptr` 用**引用计数**实现共享。代价：大小 = 2 个指针（对象 + 控制块），拷贝/析构有**原子操作**开销。理解控制块是安全使用 `shared_ptr` 的关键。

---

## 核心机制

控制块（control block）含：强引用计数 + 弱引用计数 + 删除器 + 分配器。

```cpp
auto sp = std::make_shared<Widget>();  // sp 引用计数 = 1
auto sp2 = sp;                          // 拷贝，计数 = 2（原子 ++）
// sp2 销毁 → 计数 = 1
// sp 销毁 → 计数 = 0 → delete Widget
```

### 控制块创建时机（只创建一次）

1. `make_shared`（推荐）—— 单次分配
2. 从 `unique_ptr` 构造
3. 用裸指针 `new` 构造 `shared_ptr`

**致命错误**：两次 `new` 同一裸指针构造两个 `shared_ptr` → 两个控制块 → **double free**：
```cpp
Widget* p = new Widget;
auto sp1 = shared_ptr<Widget>(p);
auto sp2 = shared_ptr<Widget>(p);  // double free！
```

### enable_shared_from_this

对象需要在成员函数里返回自身的 `shared_ptr` 时：
```cpp
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> getPtr() { return shared_from_this(); }
};
// 直接 shared_ptr<Widget>(this) 会创建新控制块 → double free
```

---

## 新手要点（和 C 的区别）

- **C 没有引用计数智能指针**：C 需要手动管理引用计数（如 Linux 内核的 `kref`）。C++ 的 `shared_ptr` 自动管理。
- **代价**：`shared_ptr` 拷贝/析构有原子操作开销（比裸指针慢一个数量级）。别在热路径用。
- **`make_shared` 优先**：一次分配（对象 + 控制块），比 `shared_ptr<T>(new T)` 的两次分配更省且 cache 友好。

---

## HFT 关联

- **热路径绝不用**：`shared_ptr` 的原子计数在多核间引发 cache 行同步，每 tick 路径上拷贝 `shared_ptr` 是性能灾难。
- **对象池/策略共享**：非热路径（对象池、策略对象生命周期管理）可以用，但在线程入口一次性拷贝，热循环内用裸引用。

---

## 自测题

1. `shared_ptr` 的大小是多少？为什么比 `unique_ptr` 大？
2. 用裸指针 `new` 构造两个 `shared_ptr` 会发生什么？
3. `enable_shared_from_this` 解决什么问题？为什么不能直接 `shared_ptr<T>(this)`？
4. `shared_ptr` 拷贝的开销是什么？为什么 HFT 热路径不用它？

---

## 参考与延伸

- 下一节：[Item 20 weak_ptr](item20-weak-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
