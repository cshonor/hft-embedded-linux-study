# 条款 18：接口设计要易用正确、难被误用

## 本节讲什么

**Make interfaces easy to use correctly and hard to use incorrectly.** 理想接口：**误用应无法通过编译**；能通过编译的用法，运行时就应是用户所期望的行为。实现手段包括：**强类型**、**限制操作**、**与内置类型行为一致**、**替客户封装资源管理**（智能指针 + 自定义 deleter）。

← [条款 17：独立语句存 smart_ptr](../ch03-resource-management/item17-用独立语句把new出来的对象存入智能指针.md) · [条款 19：类即新类型 →](./item19-设计类等同于设计一种全新类型.md)

---

## 设计目标

| 层次 | 含义 |
|------|------|
| **编译期** | 参数顺序错、非法值、资源泄漏责任推给客户 → **尽量变成编译错误** |
| **运行期** | 无法编译期拦截的误用 → 断言、异常、文档；但**不能**依赖客户「记得做某事」 |
| **一致性** | 自定义类型行为接近 `int`、STL 等已有约定 → 降低记忆成本 |

---

## 1. 引入新类型，预防参数传递错误

### 问题：`int` 参数易混

```cpp
// ❌ 月/日/年全是 int：顺序颠倒、Month=30 都能编译
Date(int month, int day, int year);
Date(30, 3, 1995);   // 编译通过，语义错误
Date(3, 30, 1995);   // 若用户记错顺序，同样编译通过
```

### 解决：强类型包装

```cpp
struct Day   { explicit Day(int d) : val(d) { /* 可校验 1..31 */ } int val; };
struct Month { explicit Month(int m) : val(m) { /* 可校验 1..12 */ } int val; };
struct Year  { explicit Year(int y) : val(y) {} int val; };

class Date {
public:
    Date(Month m, Day d, Year y);   // 类型不同 → 传参顺序错 = 编译错误
};

Date(Month(3), Day(30), Year(1995));  // ✅
// Date(30, 3, 1995);                 // ❌ 不能编译
```

### 进一步：静态工厂限定合法取值

裸 `Month(13)` 仍可能编译通过（若 ctor 未校验）。更严做法：**只暴露合法月份**：

```cpp
class Month {
    explicit Month(int m) : val(m) {}
public:
    static Month Jan() { return Month(1); }
    static Month Feb() { return Month(2); }
    // ... Dec()
    friend class Date;
private:
    int val;
};

Date d(Month::Mar(), Day(15), Year(2024));  // 非法月份名 → 无法表达
```

| 手段 | 防什么 |
|------|--------|
| `Day` / `Month` / `Year` | 参数顺序颠倒 |
| `explicit` 单参 ctor | 不想要的隐式转换（见 [条款 24](./item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md)） |
| `Month::Jan()` 等 | 超出合法范围的「魔法数字」 |

C++11 起也可用 **`enum class`** 表达有限集合，但 Item 18 强调的 **`Month::Jan()`** 模式在需要**运行时校验**或**非连续合法值**时仍常用。

---

## 2. 限制操作，并与内置类型保持一致

### 用 `const` 限制误操作

```cpp
class Rational {
public:
    const Rational operator*(const Rational& rhs) const;
};

Rational a, b, c;
if (a * b = c) { ... }   // ❌ 编译错误：不能给 const 对象赋值
(a * b) = c;              // ❌ 同理
```

若 `operator*` 返回非 `const`，笔误 `a * b = c`（本意 `==`）可能**编译通过**却语义荒谬。  
**让错误用法在编译期失败**，正是「难以错误使用」的典型手段。

### 与内置类型行为一致

除非有**充分理由**，自定义类型应模仿用户已熟悉的语义：

| 约定 | 例子 |
|------|------|
| 元素个数叫 `size()` | STL 容器统一；勿有的叫 `length()`、有的叫 `count()` 除非领域惯例明确 |
| 赋值返回 `*this` | 与 `int` 链式赋值一致（[条款 10](../ch02-constructors-destructors-assignment/item10-令operator=返回引用指向this对象.md)） |
| 拷贝/移动语义符合直觉 | 值类型深拷、资源句柄转移（[条款 14](../ch03-resource-management/item14-资源管理类谨慎设计拷贝行为.md)） |

**一致性 = 少记一套规则 = 少出错。**

---

## 3. 消除客户的资源管理职责

任何要求客户**「记得 delete / close / unlock」** 的接口，都**容易**泄漏或 double-free。

```cpp
// ❌ 工厂返回裸指针：客户必须记得 delete，且不能 delete 两次
Investment* createInvestment();
```

**接口设计者先发制人**：

```cpp
std::shared_ptr<Investment> createInvestment();
// 或 sole ownership：
std::unique_ptr<Investment> createInvestment();
```

- 资源**默认**进入 RAII（[条款 13](../ch03-resource-management/item13-以对象管理资源（RAII）.md)）。  
- 工厂内部 `new` 时仍遵守 [条款 17](../ch03-resource-management/item17-用独立语句把new出来的对象存入智能指针.md)：**独立语句** + 优先 `make_shared` / `make_unique`。

客户侧：

```cpp
auto inv = createInvestment();  // 无需、也不应手动 delete
```

---

## 4. 智能指针 + 自定义 deleter：进阶防护

### 绑定正确的释放机制

资源**不能**用普通 `delete` 释放时，把**正确释放函数**绑进 `shared_ptr`：

```cpp
void getRidOfInvestment(Investment* p);

std::shared_ptr<Investment> createInvestment() {
    struct Deleter {
        void operator()(Investment* p) const {
            getRidOfInvestment(p);   // 非 delete
        }
    };
    Investment* raw = /* ... */;
    return std::shared_ptr<Investment>(raw, Deleter{});
    // 现代写法：return { raw, Deleter{} };
}
```

客户**不可能**误用 `delete`——释放策略由接口**唯一**指定（与 [条款 14](../ch03-resource-management/item14-资源管理类谨慎设计拷贝行为.md) 自定义 deleter 一脉相承）。

### 防范 cross-DLL 问题

典型场景：对象在 **DLL A** 中 `new`，在 **DLL B** 中 `delete` → 可能**运行时崩溃**（不同堆/ABI）。

`shared_ptr` 的 **deleter 与分配点绑定**（默认 deleter 会记录「在哪个模块 delete」），工厂在**同一 DLL** 内构造 `shared_ptr` 并返回，客户在任何模块**只持有智能指针**，由**正确模块**执行释放 → 规避 cross-DLL 陷阱。

> 现代工程仍优先：**模块边界清晰 + 工厂返回智能指针**；Windows/Linux 插件场景尤其值得默认这样做。

---

## 策略对照总表

| # | 策略 | 典型手段 | 拦截阶段 |
|---|------|----------|----------|
| 1 | 强类型 | `Day`/`Month`/`Year`、`Month::Jan()` | 编译期 |
| 2 | 限制操作 | `const` 返回值、`explicit` | 编译期 |
| 2 | 行为一致 | `size()`、赋值返回 `*this` | 习惯 / 少错 |
| 3 | 接管资源 | 工厂返回 `shared_ptr` / `unique_ptr` | 编译期 + RAII |
| 4 | 正确释放 | 自定义 deleter；同 DLL 构造 `shared_ptr` | 运行期安全 |

---

## HFT 视角

- **订单 ID / 合约代码**：用 `struct OrderId { explicit OrderId(uint64_t); }` 而非裸 `uint64_t`，避免与 `price`、`qty` 传反。  
- **行情句柄 / 共享内存**：工厂返回 `unique_ptr<MarketDataFeed, CustomDeleter>`，禁止 `Feed*` 裸出。  
- **跨 .so 插件**：策略插件只拿 `shared_ptr<Interface>`，不在插件外 `delete` 宿主分配的对象。  
- API review 清单：**有没有裸 `new` 返回值？有没有三个连续 `int`/`double` 参数？**

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `Date(int,int,int)` 参数可交换 | 强类型或 `enum class` + 命名工厂 |
| `operator*` 可赋值 | 返回 `const` 结果 |
| 工厂 `T*` 交给调用方 delete | 返回 `unique_ptr` / `shared_ptr` |
| 应用 `delete` 释放应用 `getRidOf` 的资源 | deleter 写进 `shared_ptr` |
| 每个类一套 `length`/`size`/`count` | 对齐 STL / 领域统一命名 |

---

## 核心总结

1. **易用且难误用**：误用优先 **编译失败**；能编译则应 **语义正确**。  
2. **强类型**（`Month::Jan()`）阻断顺序错、非法值。  
3. **`const` + 与内置类型一致** 减少笔误与记忆负担。  
4. **工厂返回智能指针**，把 [条款 13](../ch03-resource-management/item13-以对象管理资源（RAII）.md) 的责任收回到接口内；配合 [条款 17](../ch03-resource-management/item17-用独立语句把new出来的对象存入智能指针.md) 安全构造。  
5. **自定义 deleter** 绑定正确释放方式，并缓解 **cross-DLL** 问题。  
6. 完整「类型即契约」见 [条款 19](./item19-设计类等同于设计一种全新类型.md)。

← [条款 17：独立语句存 smart_ptr](../ch03-resource-management/item17-用独立语句把new出来的对象存入智能指针.md) | [条款 19：类即新类型 →](./item19-设计类等同于设计一种全新类型.md)
