# A.3 智能指针

> 附录 A · 上一节：[A.2 lambda 表达式](02-lambda.md) · 下一节：[A.4 auto 与 decltype](04-auto-decltype.md)

## 这节讲什么

C++11 引入 `unique_ptr`、`shared_ptr`、`weak_ptr` 三种智能指针——自动管理内存，告别 `new`/`delete`。本节讲三者的区别、所有权语义、以及在并发中的使用注意。

---

## 核心规则（代码+表格）

### 三种智能指针

| 指针 | 所有权 | 开销 | 可拷贝 | 并发安全 |
|------|--------|------|--------|---------|
| `unique_ptr` | 独占 | 零（== 裸指针） | 否（可 move） | 否（需同步） |
| `shared_ptr` | 共享（引用计数） | 原子计数 | 是 | 计数安全，对象不安全 |
| `weak_ptr` | 不拥有（观察） | 小 | 是 | 配合 shared_ptr |

### `unique_ptr`：零开销独占

```cpp
// 独占所有权，析构自动 delete
std::unique_ptr<Buffer> p = std::make_unique<Buffer>(1024);
// p 析构 → 自动 delete Buffer

// 不能拷贝，只能 move
std::unique_ptr<Buffer> p2 = std::move(p);  // p 变 nullptr
// std::unique_ptr<Buffer> p3 = p;  // 编译错误

// 自定义删除器（如 mempool 归还）
auto deleter = [](Buffer* b){ return_to_pool(b); };
std::unique_ptr<Buffer, decltype(deleter)> p(new Buffer(), deleter);

// unique_ptr == 裸指针性能（零开销抽象）
static_assert(sizeof(std::unique_ptr<Buffer>) == sizeof(Buffer*));  // 通常成立
```

### `shared_ptr`：共享所有权

```cpp
// 引用计数管理
auto p1 = std::make_shared<Buffer>(1024);  // count=1
auto p2 = p1;  // 拷贝 → count=2
// p1 和 p2 都析构 → count=0 → delete Buffer

// make_shared 比 new + shared_ptr 高效
auto p = std::make_shared<Buffer>(1024);  // 一次分配（对象+计数）
auto q = std::shared_ptr<Buffer>(new Buffer(1024));  // 两次分配

// 线程安全性的精细区分
auto sp = std::make_shared<int>(0);
// 线程1：sp = std::make_shared<int>(1);  // 写 sp 本身 → 不安全！
// 线程2：auto local = sp;                 // 读 sp 本身 → 不安全！
// 但：
// 线程1：sp->fetch_add(1);  // 操作 *sp → 安全（如果 *sp 是 atomic）
// 线程2：sp->load();        // 操作 *sp → 安全
```

### `weak_ptr`：打破循环引用

```cpp
// 循环引用 → 内存泄漏
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;  // 强引用 → 循环
};
// 两个 Node 互相引用 → count 永不归零 → 泄漏

// 用 weak_ptr 打破循环
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // 弱引用 → 不增加计数
    void use_prev() {
        if (auto p = prev.lock()) {  // lock() 升级为 shared_ptr
            p->do_something();       // 安全：p 有效
        }
    }
};
```

### `enable_shared_from_this`

```cpp
class Worker : public std::enable_shared_from_this<Worker> {
public:
    void start() {
        // 错误：return shared_ptr<Worker>(this);  // 双重管理 → double-free
        // 正解：return shared_from_this();
        auto self = shared_from_this();
        std::thread t([self]{ self->work(); });  // 持有 shared_ptr
        t.detach();
    }
    void work() { /* ... */ }
};
auto w = std::make_shared<Worker>();
w->start();  // 线程持有 w 的 shared_ptr → 安全
```

---

## 新手要点（和 C 的区别）

- **C 程序员习惯 `malloc`/`free`**：C++ 的智能指针让手动 `delete` 成为过去——RAII 自动管理。C 程序员转型时要改掉 `new`/`delete` 配对的习惯，改用 `make_unique`/`make_shared`。
- **`unique_ptr` 零开销**：C 程序员可能担心智能指针有性能开销——`unique_ptr` 没有。它和裸指针一样大、一样快，只是析构时自动 `delete`。HFT 可以放心用。
- **`shared_ptr` 的原子计数有代价**：C 程序员可能觉得"共享指针很方便"——但 `shared_ptr` 的引用计数是原子操作（`atomic`），每次拷贝/析构有 cache bounce。HFT 热路径避免 `shared_ptr`。
- **`weak_ptr` 是 C 程序员陌生概念**：C 里没有"弱引用"——要手动管理生命周期。C++ 的 `weak_ptr` 可以安全地"观察"一个可能已销毁的对象——`lock()` 失败返回空 `shared_ptr`。

---

## HFT 关联

- **`unique_ptr` 是 HFT 的默认选择**：零开销、独占所有权、自动释放——HFT 中凡是指针都优先 `unique_ptr`，包括 mempool 管理（自定义删除器）。
- **`shared_ptr` 在 HFT 中的场景**：策略热切换（主线程替换策略，工作线程持有旧策略直到用完）——用 `shared_ptr<Strategy>` + `atomic` 读写。但要计数开销。
- **`weak_ptr` 用于缓存**：HFT 的行情缓存用 `weak_ptr` 观察可能被回收的历史数据——`lock()` 失败说明已过期。
- **`enable_shared_from_this` 在 HFT 回调中**：HFT 的异步回调如果需要安全地访问 `this`，用 `shared_from_this`——避免对象在回调执行前析构。
- **`make_shared` 一次分配**：HFT 中 `make_shared` 比 `shared_ptr(new T)` 更好——一次堆分配（对象+计数一起），减少 malloc 调用。

---

## 自测题

1. `unique_ptr`、`shared_ptr`、`weak_ptr` 的所有权语义有什么区别？
2. 为什么 `unique_ptr` 是"零开销"的？它和裸指针有什么区别？
3. `shared_ptr` 的引用计数是原子的，为什么"读写 shared_ptr 本身"还是不安全？
4. `weak_ptr` 如何打破循环引用？`lock()` 做了什么？
5. `enable_shared_from_this` 解决了什么问题？为什么不能 `shared_ptr<T>(this)`？

---

## 参考与延伸

- 下一节：[A.4 auto 与 decltype](04-auto-decltype.md)
- 上一节：[A.2 lambda 表达式](02-lambda.md)
- 回到：[附录 A](README.md)
