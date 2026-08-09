# Item 21：优先 make_unique / make_shared 而非 new

> 第 4 章 智能指针 · Item 21 · 上一节：[Item 20 weak_ptr](item20-weak-ptr.md)

## 为什么要学这个（先建立直觉）

C 程序员分配内存的方式：

```c
struct Widget* p = malloc(sizeof(struct Widget));
if (p) { widget_init(p, args); }
// ... 使用 ...
free(p);  // 手动释放
```

C++ 用 `new`/`delete` 替代 `malloc`/`free`，加上智能指针自动释放：

```cpp
// 方式 1：new + 包装（两次分配）
std::shared_ptr<Widget> sp(new Widget(args));
// → new 分配 Widget 一次，shared_ptr 分配控制块一次 = 2 次堆分配

// 方式 2：make_shared（一次分配）
auto sp = std::make_shared<Widget>(args);
// → 一次分配同时建 Widget + 控制块 = 1 次堆分配
```

`make_shared` 不仅少一次分配（更快、更 cache 友好），还解决了异常安全问题。

---

## 这节讲什么

`make_shared` 一次分配同时建对象 + 控制块，比 `shared_ptr<T>(new T)` 的两次分配更省且 cache 友好。但有些场景不能用 `make`。

---

## make 的优势

### 单次堆分配

```cpp
auto sp = std::make_shared<Widget>(args);           // 1 次分配
auto sp2 = std::shared_ptr<Widget>(new Widget(args));  // 2 次分配
// make_shared 把 Widget 对象和控制块放在同一块内存里
// → 更少的 malloc 调用、更少的内存碎片、更好的 cache 局部性
```

### 异常安全

```cpp
// 危险：new 和 may_throw 的求值顺序未指定
func(std::shared_ptr<T>(new T), may_throw());
// 可能的执行顺序：new T → may_throw() → 构造 shared_ptr
// 如果 may_throw 抛异常 → new T 的内存泄漏！

// 安全：make 保证不会泄漏
func(std::make_shared<T>(), may_throw());
// make_shared 内部一次完成分配+构造+包装，不会中间被打断
```

### 代码简洁

```cpp
// 啰嗦
std::shared_ptr<Widget> sp(new Widget(42));
std::unique_ptr<Widget> up(new Widget(42));

// 简洁
auto sp = std::make_shared<Widget>(42);
auto up = std::make_unique<Widget>(42);  // C++14
```

---

## 不能用 make 的场景

```cpp
// 1. 自定义删除器——make 不支持
std::shared_ptr<FILE> fp(fopen("data.txt", "r"), fclose);
// make_shared 无法传自定义删除器

// 2. 自定义分配器
std::shared_ptr<T> sp(std::allocate_shared<T>(alloc, args));

// 3. 大对象 + 长期 weak_ptr
// make_shared 把对象和控制块绑在一起——对象析构后内存延迟到所有 weak_ptr 销毁才回收
// 如果对象很大且有长期存活的 weak_ptr → 内存延迟释放
auto sp = std::make_shared<BigData>();  // BigData 析构后内存仍被 weak_ptr 持有
std::weak_ptr<BigData> wp = sp;  // 长期持有
sp.reset();  // BigData 析构了，但内存没释放（等 wp 销毁）
```

---

## 常见错误（新手踩坑）

**错误 1：函数参数中 new + 异常泄漏**
```cpp
void process(std::shared_ptr<Widget> sp, int priority);
process(std::shared_ptr<Widget>(new Widget), compute_priority());
// 如果 compute_priority 先执行且抛异常 → Widget 泄漏
```
**修正：** `process(std::make_shared<Widget>(), compute_priority());`

**错误 2：大对象 + 长期 weak_ptr 用 make_shared**
```cpp
auto sp = std::make_shared<HugeBuffer>();  // 10MB
std::weak_ptr<HugeBuffer> monitor = sp;    // 监控对象长期持有
sp.reset();  // HugeBuffer 析构了，但 10MB 内存没释放！
```
**修正：** 用 `std::shared_ptr<HugeBuffer>(new HugeBuffer)` 分离对象和控制块。

**错误 3：用 new 传给 make_shared**
```cpp
auto sp = std::make_shared<Widget>(new Widget());  // 两次 new！
// make_shared 会在内部 new，你又传了一个 new 的对象
```
**修正：** `auto sp = std::make_shared<Widget>();`（传构造参数，不是 new 的对象）

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 分配 | `malloc` | `new` / `make_shared` | C++ 有构造函数 |
| 释放 | `free` | `delete` / 智能指针自动 | RAII |
| 异常安全 | 不适用 | `make_shared` 保证 | 异常安全 |
| 分配次数 | 1 次 `malloc` | `make_shared` 1 次，`new` 2 次 | 控制块 |

**一句话总结：** C 程序员记住——`make_unique`/`make_shared` 是"分配+构造+包装"一步到位的安全工厂函数。优先用它们，除非需要自定义删除器或大对象+长期 weak_ptr。

---

## HFT 关联

- **cache 友好**：`make_shared` 的单次分配让对象和控制块在同一 cache 行，减少 cache miss。
- **异常安全**：HFT 代码在热路径中传 `shared_ptr` 给函数时用 `make_shared` 避免异常泄漏。
- **减少分配次数**：每减少一次 `malloc` 都能减少锁竞争和内存碎片——HFT 低延迟场景每次微秒都重要。

---

## 自测题

1. `make_shared` 比 `shared_ptr<T>(new T)` 省在哪里？
2. `func(shared_ptr<T>(new T), may_throw())` 有什么异常安全问题？`make_shared` 如何解决？
3. 什么场景下不能用 `make_shared` 而要用 `new`？
4. 为什么大对象 + 长期 `weak_ptr` 不适合 `make_shared`？
5. 下面代码有什么问题？
```cpp
auto sp = std::make_shared<Widget>(new Widget(42));
```

---

## 参考与延伸

- 下一节：[Item 22 Pimpl](item22-pimpl.md)
- 回到：[第 4 章 智能指针](README.md)
