# 条款 13：以对象管理资源（RAII）

## 本节讲什么

**Use objects to manage resources.** 工厂返回 **`new` 指针 + 手动 `delete`** 极易泄漏：`return`、`continue`、`goto`、**异常** 任一提前退出都可能跳过 `delete`。对策：**RAII** —— 资源交给**栈上对象**，靠 **析构函数自动释放**（[条款 8](../ch02-constructors-destructors-assignment/item08-别让异常逃离析构函数.md) 保证析构路径可靠）。

← [条款 12：拷贝全部成员](../ch02-constructors-destructors-assignment/item12-复制对象时勿忘其每一个成员.md) · 拷贝智能指针见 [条款 14](./item14-资源管理类谨慎设计拷贝行为.md)

---

## 手动 `delete` 为何不可靠

```cpp
Investment* createInvestment();   // 工厂：调用者 delete

void f() {
    Investment* pInv = createInvestment();
    // ... 可能 return / continue / throw ...
    delete pInv;                    // 任何提前退出 → 泄漏
}
```

泄漏的不只是堆块，还有对象内 **文件句柄、锁、DB 连接** 等。

---

## RAII 两条准则

### 1. 获得资源后，立即交给资源管理对象

**Resource Acquisition Is Initialization**：获取资源与初始化管理对象**同一语句**完成，建立「防波堤」。

```cpp
void f() {
    std::unique_ptr<Investment> pInv(createInvestment());
    // 使用 *pInv / pInv->...
}   // 离开作用域 → 自动 delete，无论正常还是异常退出
```

### 2. 析构函数负责释放

局部对象销毁时编译器**必定**调用析构（栈展开同样调用）→ 资源**总会**被释放。

| 资源类型 | RAII 示例 |
|----------|-----------|
| 堆内存 | `unique_ptr` / `shared_ptr` |
| 互斥锁 | 构造 `lock`、析构 `unlock` |
| 文件 | `fstream`、自定义 `FileGuard` |
| 数据库连接 | 连接 Guard + 显式 `close()`（条款 8） |

```cpp
class Lock {
    std::mutex& m;
public:
    explicit Lock(std::mutex& mu) : m(mu) { m.lock(); }
    ~Lock() { m.unlock(); }           // RAII：作用域结束必 unlock
};
// 现代 C++：优先 std::lock_guard / std::unique_lock
```

---

## 堆资源：智能指针

原书 **`std::auto_ptr`**、**`tr1::shared_ptr`**；现代标准对应关系：

| 原书 | 现代（C++11+） | 拷贝语义 |
|------|----------------|----------|
| `auto_ptr` | **`std::unique_ptr`**（C++17 已移除 `auto_ptr`） | **移动独占**：拷贝 = 转移所有权，源变空 |
| `tr1::shared_ptr` | **`std::shared_ptr`** | **共享**：拷贝增加引用计数，计数为 0 时 `delete` |

### `unique_ptr`（替代 auto_ptr）

```cpp
std::unique_ptr<Investment> p1(createInvestment());
auto p2 = std::move(p1);    // 所有权转移，p1 变为 nullptr
// std::unique_ptr p3 = p1; // ❌ 禁止拷贝
```

- 独占所有权，**不能**拷贝（只能 move）  
- 可用于 STL 容器（存 `unique_ptr` 需 move）  
- **`auto_ptr` 的「拷贝置空」** 曾导致容器与 API 陷阱 → **已废弃**

### `shared_ptr`（引用计数）

```cpp
std::shared_ptr<Investment> sp1(createInvestment());
std::shared_ptr<Investment> sp2 = sp1;   // 引用计数 +1
// sp1、sp2 都销毁后 → delete
```

- 拷贝直观，**多处共享**同一资源  
- 默认线程安全：**引用计数**原子操作（所指对象本身仍需自行同步）  
- 通常比 `unique_ptr` 开销大；**能独占就用 `unique_ptr`**（[条款 14](./item14-资源管理类谨慎设计拷贝行为.md)）

```cpp
// 原书风格（了解即可）
// std::auto_ptr<Investment> pInv(createInvestment());
// std::tr1::shared_ptr<Investment> sp(createInvestment());

// 现代写法
auto pInv = std::make_unique<Investment>();   // 条款 17：make 优于裸 new
auto spInv = std::make_shared<Investment>();
```

---

## 陷阱：不要用默认 deleter 管理 `new[]` 数组

`auto_ptr` / `shared_ptr`（默认 deleter）内部是 **`delete`**，不是 **`delete[]`**：

```cpp
std::shared_ptr<int> bad(new int[100]);   // ❌ 编译过，UB
std::auto_ptr<int> alsoBad(new int[100]); // ❌ 同上（历史代码）
```

| 需求 | 做法 |
|------|------|
| 动态数组 | **`std::vector`**、**`std::string`** |
| 必须堆数组 + RAII | C++11 **`std::unique_ptr<T[]>`** / **`std::shared_ptr` + 自定义 deleter**；Boost **`scoped_array` / `shared_array`**（原书） |

```cpp
std::vector<int> v(100);                  // ✅ 首选
std::unique_ptr<int[]> arr(new int[100]); // ✅ delete[]
```

---

## 与前面条款的关系

| 条款 | 关联 |
|------|------|
| **8** | 析构不抛异常 → RAII 在异常路径仍释放 |
| **11–12** | 手写资源类仍需正确 copy/assign；智能指针帮你封装 |
| **17** | `new` 与智能指针同一语句，防异常窗口 |

---

## HFT 视角

- **行情对象、订单缓冲**：工厂返回 **`unique_ptr`** 或值类型 / **`vector`**，禁止裸 `new` + 长路径手动 `delete`。  
- **会话、锁**：`lock_guard` RAII；连接用 Guard + `close()`（条款 8）。  
- **共享只读配置**：`shared_ptr<const Config>`；热路径独占缓冲用 **`unique_ptr` 或栈上数组**。  
- **绝不用** `shared_ptr<T>` 管 `new T[n]`。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 多个 exit 路径各写 `delete` | 一个 RAII 对象包起来 |
| 沿用 `auto_ptr` | 改 **`unique_ptr`** |
| 凡事 `shared_ptr` | 默认 **`unique_ptr`**，真要共享再 `shared_ptr` |
| `shared_ptr` + `new[]` | **`vector`** 或 `unique_ptr<T[]>` |
| 裸 `new` 再赋给智能指针分两行 | [条款 17](./item17-用独立语句把new出来的对象存入智能指针.md) 同一语句 |

---

## 核心总结

1. **手动 `delete` 不可靠** —— 控制流 + 异常会跳过释放。  
2. **RAII**：获取资源 **当即** 交给管理对象；**析构** 释放。  
3. 堆对象：**`unique_ptr`（独占）**、**`shared_ptr`（共享）**；优先 `make_unique` / `make_shared`。  
4. **`auto_ptr` 已淘汰**；理解其「拷贝置空」是为读懂老代码。  
5. **动态数组** → `vector` / `string` / `unique_ptr<T[]>`，勿默认 deleter + `new[]`。

← [条款 12：拷贝全部成员](../ch02-constructors-destructors-assignment/item12-复制对象时勿忘其每一个成员.md) | [条款 14：资源管理类拷贝 →](./item14-资源管理类谨慎设计拷贝行为.md)
