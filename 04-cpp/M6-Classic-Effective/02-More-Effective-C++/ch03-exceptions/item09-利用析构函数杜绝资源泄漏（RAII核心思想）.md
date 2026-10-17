# 条款 09：利用析构函数杜绝资源泄漏（RAII 核心思想）

## 本节讲什么

**Use destructors to prevent resource leaks.** 这是 More Effective C++ 异常章的开篇，
也是整个 C++ 资源管理的**第一性原理**：异常随时可能从任何一行飞出，
靠人肉 `delete`/`release` 配对守不住——把资源的释放**绑定到对象析构**上，
让栈展开（18.1 ① 实测）成为自动清理的引擎。这就是 RAII。

← 上一章 [ch02 运算符](../ch02-operators/)；下一条 [item10 构造函数抛异常](./item10-构造函数抛出异常时，如何防止内存资源泄漏.md)。

---

## 1. 问题：人肉配对的三个失守点

```cpp
void process() {
    Resource* r = acquire();
    use(r);                 // 失守点①：use 抛异常 → delete 永远到不了
    if (early) return;      // 失守点②：早退路径忘了 delete
    delete r;               // 失守点③：重构时有人在这行前面加了新 return
}
```

三个失守点的共同根源：**释放责任依赖"执行流恰好经过 delete 那一行"**——
而异常和分支让执行流千变万化，人肉穷举所有路径注定失败。

## 2. RAII：把释放绑定到析构（解法本身）

```cpp
class ResourceGuard {
    Resource* r_;
public:
    explicit ResourceGuard(Resource* r) : r_(r) {}
    ~ResourceGuard() { release(r_); }          // 释放写进析构
    Resource* get() const { return r_; }
    // 禁拷贝（或实现移动语义）——所有权必须唯一
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
};

void process() {
    ResourceGuard g(acquire());
    use(g.get());           // 随便抛
    if (early) return;      // 随便退
}                           // g 析构 → release() 必然执行（栈展开也走这里，18.1 ①）
```

**RAII 的完整表述**：资源获取即初始化（构造函数绑定资源），
资源释放即析构（析构函数释放资源）——
对象的**生存期**就是资源的**生存期**，而对象生存期由作用域/栈展开**机械保证**。

## 3. 从手写 Guard 到标准设施（别重复造）

| 资源 | 标准 RAII 设施 | 替代手写 |
|---|---|---|
| 堆内存 | `std::unique_ptr` / `shared_ptr` | ✅ 永远别手写指针 Guard |
| 锁 | `std::lock_guard` / `unique_lock` / `scoped_lock` | ✅ |
| 文件 | `std::fstream`（析构关文件） | ✅ |
| 自定义资源（fd/句柄/网卡模式） | 手写轻 Guard 或 `std::unique_ptr<T, Deleter>` | Deleter 定制释放逻辑 |

**手写 Guard 的现代替代**：`unique_ptr<void, void(*)(void*)>` 配自定义 deleter，
或 C++23 `std::expected` 错误通道 + 作用域守卫库（`gsl::finally`/`scope_exit` 模式）。

## 4. 与异常安全的联动（item29 的根基）

- RAII 是**基本保证**的实现手段（不泄漏）——没有 RAII 谈异常安全是空中楼阁
- 析构函数**必须不抛**（18.1 ②：栈展开中再抛 = terminate）——
  RAII 的析构里做"可能失败"的操作（flush/close 写盘）要 catch 住或记录后吞
- 配对记忆：**获得资源的那一刻就把它交给 RAII 对象**——
  "裸指针存活窗口"（acquire 到入 Guard 之间）越短越好，最好是零行

## HFT 关联

- 热路径的 RAII 是**零成本**的：Guard 无堆分配（对象本体在栈上）、
  析构是编译期确定的单条调用——比异常路径的 delete 快且确定
- 冷路径资源（连接/文件/网卡混杂模式）全部 RAII 化：
  重连逻辑里"旧连接的释放"交给析构，状态机只关心新连接——
  人肉配对在重连风暴里必然漏
- `unique_ptr<T, Deleter>` 是池化资源的桥：deleter 写"归还池"而非 delete——
  池化对象也能享受 RAII 的自动归还（→ 19.1 池化 + item49）

## 代码自测

**题目 1：** 为什么 RAII 能覆盖"人肉 delete"守不住的所有路径？

<details>
<summary>参考答案</summary>

因为它把释放责任从"执行流经过 delete 行"转移到**对象析构**——
而局部对象的析构由作用域规则**机械保证**：正常 return、早退 return、
异常栈展开（18.1 ① 实测逐帧析构）——**任何**离开作用域的方式都会触发析构。
人肉配对要穷举所有路径（异常路径还根本写不出来——
`delete` 无法放在"每处可能抛异常的调用"之后），
RAII 只需写一次析构。这就是"机制保证"对"纪律保证"的碾压。

</details>

**题目 2：** RAII 对象的析构里做"可能失败"的清理（如 flush 写盘）有什么风险？怎么处理？

<details>
<summary>参考答案</summary>

风险：析构在**栈展开中**运行时（18.1 ②）——此时再抛异常 = `std::terminate`。
flush/close 这类操作是可能失败的（磁盘满/网络断）。
处理三选一：① 析构里 try-catch 吞掉 + 记录（`try { flush(); } catch (...) { log_fallback(); }`）；
② 把"可能失败的收尾"提为显式方法（`guard.commit()`），
析构只做"绝不失败"的兜底（释放不提交）——用户忘了 commit 也不泄漏；
③ noexcept 析构 + 错误经带外通道上报（原子错误标志）。
交易系统选②的多：提交语义必须显式（落盘/发送是可失败的事务，不该藏在析构里）。

</details>

**题目 3：** 池化对象（`pool.acquire()` 获得，要 `pool.release(p)` 归还）怎么用 RAII 管理？

<details>
<summary>参考答案</summary>

用 `unique_ptr<T, Deleter>` 定制删除器（→ item39 ③ 的 EBO 关联）：

```cpp
struct PoolDeleter {
    Pool* pool;
    void operator()(Order* p) const noexcept { p->~Order(); pool->release(p); }
};
using PooledOrder = std::unique_ptr<Order, PoolDeleter>;
PooledOrder o{ pool.acquire(), PoolDeleter{&pool} };
```

注意 deleter 的职责是**先析构再归还池**（池内存不 delete，→ 19.1 ③ 的铁律）。
无状态 deleter 享受 EBO（unique_ptr 仍 8 字节，→ item39 实测）；
带 Pool* 的版本涨到 16 字节——热路径大量持有时，把 Pool 设为全局/单例
让 deleter 无状态化（item39 ③ 的尺寸账）。

</details>
