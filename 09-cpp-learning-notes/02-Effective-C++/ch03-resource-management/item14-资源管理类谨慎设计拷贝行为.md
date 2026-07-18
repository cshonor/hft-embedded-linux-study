# 条款 14：资源管理类谨慎设计拷贝行为

## 本节讲什么

**Think carefully about copying behavior in resource-managing classes.** [条款 13](./item13-以对象管理资源（RAII）.md) 的 RAII / 智能指针主要管 **堆资源**；**锁、文件句柄、socket** 等往往要**自定义 RAII 类**。一旦允许拷贝，必须回答：**拷贝 RAII 对象 = 拷贝其管理的资源 —— 资源应如何被共享或转移？**

← [条款 13：RAII](./item13-以对象管理资源（RAII）.md) · 禁止拷贝见 [条款 6](../ch02-constructors-destructors-assignment/item06-不想用编译器自动生成的函数，就明确禁用.md)

---

## 核心问题

```cpp
class Lock {
    std::mutex& m;
public:
    explicit Lock(std::mutex& mu) : m(mu) { m.lock(); }
    ~Lock() { m.unlock(); }
};
```

`Lock a(m); Lock b(a);` —— **拷贝一把锁**在语义上通常**无意义**。  
设计 RAII 类时，**拷贝构造 / 拷贝赋值**必须与**底层资源的语义**一致。

> **底层资源怎么拷，RAII 对象就怎么拷。**

---

## 四种设计选择

| # | 策略 | 何时用 | 典型手段 |
|---|------|--------|----------|
| **1** | **禁止拷贝** | 资源不可共享、不可转移语义（锁、独占 fd） | `= delete`、[条款 6](../ch02-constructors-destructors-assignment/item06-不想用编译器自动生成的函数，就明确禁用.md) |
| **2** | **引用计数** | 多对象共享，**最后一个**销毁时释放 | `shared_ptr` + 自定义 **deleter** |
| **3** | **深拷贝** | 每个副本拥有**独立**资源副本 | 手写 copy ctor/assign；如 `std::string` |
| **4** | **转移所有权** | 任一时刻**仅一个** RAII 对象持有资源 | `unique_ptr` / 原书 `auto_ptr` 式 move |

---

## 1. 禁止拷贝（Prohibit copying）

拷贝**无意义或危险**时，明确禁用：

```cpp
class Lock {
    std::mutex& m;
public:
    explicit Lock(std::mutex& mu) : m(mu) { m.lock(); }
    ~Lock() { m.unlock(); }

    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
};
```

- 互斥体 **lock**、独占 **文件描述符**、**线程** 句柄等 → 常选此策略。  
- 现代：**`= delete`**；经典：private 声明 + 不定义 / `Uncopyable` 继承。

---

## 2. 引用计数 + 自定义 deleter

希望**多个 RAII 对象共享同一资源**，直到**最后一个**消失才释放 —— 与 **`shared_ptr`** 相同模型；释放动作不一定是 `delete`。

### 堆对象：默认 deleter

```cpp
std::shared_ptr<Investment> sp(createInvestment());  // 计数为 0 → delete
```

### 非堆资源：deleter 做 `unlock`（原书 Mutex 技巧）

Mutex 本身不是 `new` 出来的，但可把 **「锁是否仍被某 Guard 持有」** 用引用计数 + **`unlock` deleter** 建模（教学示例；生产更常用 `lock_guard`）：

```cpp
void unlockMutex(std::mutex* pm) {
    if (pm) pm->unlock();
}

class Lock {
public:
    explicit Lock(std::mutex& mu)
        : sp(&mu, unlockMutex)    // deleter：计数归零时 unlock，不是 delete
    { sp->lock(); }

private:
    std::shared_ptr<std::mutex> sp;  // 不 delete mutex，只管理「解锁责任」
};
```

- **`shared_ptr` 构造的第二个参数** = 自定义 **deleter**。  
- 拷贝 `Lock` → **`shared_ptr` 拷贝** → 引用计数 +1；最后一个析构 → 调用 **`unlockMutex`**。  
- 甚至**不必写自定义析构函数** —— deleter 包办释放逻辑。

> 日常锁：直接用 **`std::lock_guard` / `unique_lock`**（不可拷贝）。引用计数 + deleter 展示的是 **shared_ptr 泛化能力**。

---

## 3. 深拷贝底层资源（Deep copy）

允许**多个独立副本**，各自在析构时释放**自己的**那份资源：

```cpp
class StringLike {
    char* data;
public:
    StringLike(const StringLike& rhs)
        : data(new char[std::strlen(rhs.data) + 1]) {
        std::strcpy(data, rhs.data);
    }
    StringLike& operator=(const StringLike& rhs) { /* 条款 11、12 */ }
    ~StringLike() { delete[] data; }
};
```

**`std::string`** 拷贝即深拷贝堆上字符（实现可能 SSO，语义仍是独立副本）。  
前提：资源**可以**且**应该**复制（内存块、配置快照），且 [条款 12](../ch02-constructors-destructors-assignment/item12-复制对象时勿忘其每一个成员.md) 拷全部分。

---

## 4. 转移所有权（Transfer ownership）

**任一时刻只有一个** RAII 对象拥有裸资源；拷贝 = **夺权**，源对象变空：

```cpp
// 原书 std::auto_ptr — 拷贝置空
// std::auto_ptr<Investment> p1(createInvestment());
// std::auto_ptr<Investment> p2(p1);  // p1 变 null

// 现代
std::unique_ptr<Investment> p1 = std::make_unique<Investment>();
auto p2 = std::move(p1);             // 所有权转移
```

- **`unique_ptr`**：明确 **move-only**，替代 `auto_ptr`。  
- 适用于**独占**语义且允许**转移**（工厂出参、容器 reseat），而非共享。

---

## 如何选择？

```
资源能否/应被共享？
  ├─ 否，且不应转移 → 禁止拷贝（Lock、lock_guard）
  ├─ 否，但可转移   → unique_ptr / move-only RAII
  ├─ 是，共享直到最后使用者 → shared_ptr（必要时 custom deleter）
  └─ 是，每份独立副本     → 深拷贝（string、值语义 buffer）
```

**最常见**：**禁止拷贝** + **引用计数**（堆对象 `shared_ptr`、特殊 deleter 管非 delete 释放）。

---

## HFT 视角

| 资源 | 建议拷贝策略 |
|------|----------------|
| **互斥锁 Guard** | **禁止拷贝** |
| **独占 socket / 会话** | **禁止拷贝** 或 **unique_ptr** 转移 |
| **共享只读行情缓存** | **`shared_ptr<const>`** |
| **订单簿快照字符串/缓冲** | **深拷贝** 或 **`vector` 值语义** |
| **连接池中的连接** | **shared_ptr** + deleter 归还池，而非 `delete` |

拷贝 RAII 前问：**多份对象是指向同一锁/同一 fd，还是各自一份？** 答错即死锁或 double-close。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `Lock` 可拷贝 → 双重 unlock | `= delete` 拷贝 |
| 非堆资源硬塞 `shared_ptr` 默认 delete | **custom deleter** 或专用 Guard |
| 该独占却用 `shared_ptr` | 改 **`unique_ptr`** |
| 该共享却深拷贝大缓冲 | **`shared_ptr`** 或只读别名 |
| 自定义 RAII 用默认 member-wise 拷贝 | 按四种策略显式设计 |

---

## 核心总结

1. **拷贝 RAII = 拷贝资源语义** —— 先定资源规则，再定类规则。  
2. **四选一（或组合）**：禁止拷贝、引用计数、深拷贝、转移所有权。  
3. **`shared_ptr` + deleter** 可统一「计数归零时 `unlock`/关闭/还池」，不限于 `delete`。  
4. 常规默认思路：**能禁止就禁止；要共享就 `shared_ptr`；要独立副本就深拷贝；要独占转移就 `unique_ptr`。**  
5. 与 [条款 15](./item15-资源管理类提供原始资源访问接口.md) 衔接：拷行为定好后，再暴露 `get()` 给 C API。

← [条款 13：RAII](./item13-以对象管理资源（RAII）.md) | [条款 15：原始资源访问 →](./item15-资源管理类提供原始资源访问接口.md)
