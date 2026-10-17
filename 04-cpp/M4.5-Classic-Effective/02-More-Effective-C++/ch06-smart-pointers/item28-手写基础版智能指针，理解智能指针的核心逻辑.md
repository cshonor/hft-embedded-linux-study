# 条款 28：手写基础版智能指针，理解智能指针的核心逻辑

## 本节讲什么

**Implement a basic smart pointer from scratch.** 手写一个 unique_ptr 教学版，
把智能指针的四块拼图（所有权、RAII、移动语义、防拷贝）拆开看清楚——
用过 `unique_ptr` 和**理解它为什么这样设计**是两回事，
后者是自定义资源包装（池化句柄/文件描述符）的基本功。

← 上一章 [ch05 面向对象设计](../ch05-oop-design/)；
下一条 [item29 引用计数智能指针](./item29-实现引用计数智能指针（shared_ptr前身），循环引用问题来源就在这一条.md)。

---

## 1. 最小教学实现（unique_ptr 的四块拼图）

```cpp
template <typename T>
class UniquePtr {
    T* ptr_ = nullptr;
public:
    // 拼图①：所有权获取——构造即接管
    explicit UniquePtr(T* p = nullptr) noexcept : ptr_(p) {}

    // 拼图②：RAII——析构即释放（→ MoreEff item09）
    ~UniquePtr() { delete ptr_; }

    // 拼图③：禁拷贝——所有权唯一，拷贝 = 双重释放的温床
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    // 拼图④：移动——所有权可以**转移**（唯一合法的所有权变更方式）
    UniquePtr(UniquePtr&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) {
            delete ptr_;              // 先释放自己持有的（自赋值安全）
            ptr_ = o.ptr_;
            o.ptr_ = nullptr;         // 源置空：所有权交接完成
        }
        return *this;
    }

    // 指针接口
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    T* release() noexcept { T* p = ptr_; ptr_ = nullptr; return p; }   // 放弃所有权
    void reset(T* p = nullptr) { delete ptr_; ptr_ = p; }              // 换持
};
```

## 2. 四块拼图各自的"为什么"

| 拼图 | 没有它会怎样 |
|---|---|
| ① explicit 构造 | `UniquePtr p = raw_ptr;` 隐式接管——谁释放谁糊涂（所有权必须显式表态） |
| ② RAII 析构 | 回到人肉 delete 的三失守点（→ MoreEff item09） |
| ③ 禁拷贝 | 两个 UniquePtr 持有同一指针 → 双重释放（double free） |
| ④ 移动 | 所有权无法传递——函数没法"产出"一个资源（要么泄漏要么返回裸指针） |

**关键洞察**：智能指针的设计本质上是**用类型系统表达所有权规则**——
"唯一所有权"在代码里不是注释，而是"拷贝构造 = delete"的编译期禁令。

## 3. 从教学版到 unique_ptr 的差距（诚实清单）

教学版缺的（知道它们存在，就知道什么时候该用真 unique_ptr）：
1. **自定义 deleter**（`unique_ptr<T, D>`——池化归还/ fclose/ dlclose，
  → MoreEff item09 ④ 的池化桥）+ **EBO**（空 deleter 零尺寸，→ Effective item39 ③）
2. **数组版**（`unique_ptr<T[]>`——delete[] 配对 + operator[]）
3. **`make_unique`**（一次分配构造，异常安全 + 少一次 new 表达式）
4. **数组/函数指针等特化** + 完整的 noexcept 标注（→ item14 语义）

## HFT 关联

- 手写资源包装的日常：**fd/会话/池槽位**——`UniqueFd`（close 析构）、
  `PooledOrder`（release 归还池，→ MoreEff item18）都是这四块拼图的实例
- 所有权规则的编译期表达（拼图③④）是**无锁设计**的前提：
  所有权唯一 = 释放点唯一 = 不需要引用计数的原子操作（→ item29 的对照）
- 池化对象的 unique 包装（`unique_ptr<T, PoolDeleter>`）在生产里的尺寸账：
  deleter 无状态（EBO 8B）vs 带池指针（16B，→ Effective item39 ③ 实测）

## 代码自测

**题目 1：** 为什么禁拷贝 + 允许移动是"唯一所有权"的完整表达？

<details>
<summary>参考答案</summary>

唯一所有权的规则是"**任何时刻只有一个 owner**"——
拷贝构造若存在：`UniquePtr a(p); UniquePtr b = a;` 后 a、b 各以为自己是
owner，析构时**双重释放**（double free）——所以拷贝必须 = delete。
但所有权必须能**流转**（函数产出资源、对象换持）——
移动构造/赋值就是流转的通道：源对象 `ptr_` 置空（所有权交出），
目标接管（唯一 owner 变更，不复制）——
"禁拷贝 + 允许移动"合起来 = "所有权唯一但可转移"的编译期表达。
这也是整个 C++11 移动语义的原型案例：资源类不可拷贝、只可移动。

</details>

**题目 2：** 移动赋值里 `if (this != &o)` 防的是什么？漏了会怎样？

<details>
<summary>参考答案</summary>

防**自移动赋值**（`p = std::move(p);`）。
漏了检查的执行序列：`delete ptr_;`（先释放自己持有的）→
`ptr_ = o.ptr_;`（o 就是自己——拿到的是**刚被释放的指针**）→
`o.ptr_ = nullptr;`（把自己置空）——结果：持有的对象被 delete，
指针置空，**资源泄漏**（对象死了没人管）或更糟的双重释放风险。
`delete ptr_` 在前的设计（先释放再接管）必须配自赋值检查；
另一种更稳的写法是 swap 后让源析构（copy-and-swap 的移动版，
→ Effective item25）。
自赋值检查是资源管理类赋值运算符的通用纪律（指针/句柄/容器同此）。

</details>

**题目 3：** 用四块拼图设计 `UniqueFd`（close 析构的文件描述符包装）。

<details>
<summary>参考答案</summary>

```cpp
class UniqueFd {
    int fd_ = -1;
public:
    explicit UniqueFd(int fd = -1) noexcept : fd_(fd) {}
    ~UniqueFd() { if (fd_ >= 0) ::close(fd_); }
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    UniqueFd(UniqueFd&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    UniqueFd& operator=(UniqueFd&& o) noexcept {
        if (this != &o) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = o.fd_; o.fd_ = -1;
        }
        return *this;
    }
    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }
};
```

与指针版的三个差异：① 资源是 int 不是指针——"空"用 -1 表示（判空逻辑相应改）；
② 释放函数是 `close` 不是 delete（deleter 概念的具象化）；
③ 没有 operator*/->（fd 不可解引用）——接口按资源语义裁剪。
四块拼图（explicit 获取 / RAII / 禁拷贝 / 移动）原样保留——
这就是"智能指针思维"泛化为"资源包装思维"的标准过程。

</details>
