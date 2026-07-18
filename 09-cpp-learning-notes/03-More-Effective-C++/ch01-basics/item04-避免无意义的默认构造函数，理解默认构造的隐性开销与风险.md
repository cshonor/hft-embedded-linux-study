# 条款 4：避免无意义的默认构造函数，理解默认构造的隐性开销与风险

## 本节讲什么

缺省构造函数（无参构造）允许在没有任何外部数据的情况下「无中生有」地创建对象。对数字类、空链表等类型这很合理；但对**必须依赖外部数据才有意义**的类（如必须带公司 ID 的 `EquipmentPiece`），无参构造只会制造「名义上存在、实际上无效」的对象。本条款说明：没有缺省构造带来的使用限制，以及为何**不应**为妥协而提供一个无意义的缺省构造。

## 何时需要缺省构造？

| 类型 | 缺省构造是否合理 |
|------|------------------|
| 行为类似数字的对象 | 合理（如 `int` 可默认初始化） |
| 空容器（链表、vector 等） | 合理（先空后填） |
| 必须带 ID / 外部配置才有意义的设备类 | **不合理**——无参构造没有意义 |

## 没有缺省构造的三处使用限制

### 1. 难以直接建立对象数组

无法写 `EquipmentPiece arr[10];`——每个元素都需要构造参数。变通手段存在，但代价高：

- **指针数组**：`EquipmentPiece *arr[10]`，逐个 `new`，内存与代码更复杂
- **placement new + 原始内存**：手动构造、**逆序析构**、释放内存，极易泄漏或析构顺序错误

```cpp
class EquipmentPiece {
    int companyId_;
public:
    explicit EquipmentPiece(int id) : companyId_(id) {}
    // 故意不提供缺省构造
};

// EquipmentPiece best[10];  // 错误：无默认构造

// 变通（复杂、易错）：
// void *buf = operator new(10 * sizeof(EquipmentPiece));
// for (int i = 0; i < 10; ++i)
//     new (static_cast<char*>(buf) + i * sizeof(EquipmentPiece)) EquipmentPiece(id[i]);
// ... 逆序手动析构 + operator delete(buf)
```

### 2. 与部分模板容器不兼容

许多模板容器在内部实例化元素数组时，**要求类型参数提供缺省构造**（C++98/03 时代尤其常见；现代标准库部分接口已放宽，但缺省构造仍广泛假设存在）。

```cpp
#include <vector>

class NoDefault {
    int id_;
public:
    explicit NoDefault(int i) : id_(i) {}
};

// std::vector<NoDefault> v(10);  // 旧标准 / 部分实现：需要默认构造
// 更稳妥：reserve + emplace_back / push_back 带参构造
```

### 3. 虚基类初始化的负担

若**虚基类**没有缺省构造，**所有**继承链上的派生类都必须理解并向虚基类传递构造参数——对派生类作者是令人厌恶的负担。

```cpp
class VirtualBase {
public:
    explicit VirtualBase(int config) {}
};

class Mid : public virtual VirtualBase {
public:
    Mid(int c) : VirtualBase(c) {}
};

class Leaf : public Mid {
public:
    Leaf(int c) : VirtualBase(c), Mid(c) {}  // 必须知道虚基类要什么
};
```

## 为何不要提供「无意义的」缺省构造？

有人为消除上述限制，给类加一个缺省构造，把对象置于「未确定（UNSPECIFIED）」的无效状态。书中**强烈反对**：

### 逻辑复杂化与状态不可控

其他成员函数必须**时刻检测**对象是否有效（如 ID 是否为未确定），并在无效时走错误分支—— invariant 从「构造保证」退化为「运行时猜测」。

### 降低效率

各处有效性检测增加分支、拖慢调用，并膨胀可执行文件/库体积。

```cpp
// 反例：无意义默认构造 + 到处检查
class BadEquipment {
    int id_;
    bool valid_;
public:
    BadEquipment() : id_(-1), valid_(false) {}           // 无意义状态
    explicit BadEquipment(int id) : id_(id), valid_(true) {}

    void operate() {
        if (!valid_) { /* 错误处理 */ return; }         // 每个函数都要测
        // ...
    }
};

// 正例：构造即有效，无需运行时「是否已初始化」检测
class GoodEquipment {
    int id_;
public:
    explicit GoodEquipment(int id) : id_(id) {}

    void operate() {
        // 可假定 id_ 始终有效
    }
};
```

## 正确思路

- **能靠构造保证全部数据正确初始化**，就不要为了「方便数组/模板」而引入无效默认状态。
- 没有缺省构造的类在使用上确有约束，但换来更强保证：**对象一旦建立，即处于有效、可高效使用的状态**。
- 需要集合时，优先 `vector` + `emplace_back` / 智能指针数组，而非强造缺省构造。

## 示例

```cpp
class EquipmentPiece {
    int companyId_;
public:
    explicit EquipmentPiece(int id) : companyId_(id) {
        // 构造完成即 invariant 成立
    }

    int id() const { return companyId_; }
    void run() const {
        // 无需 if (valid_) ...
    }
};

#include <memory>
#include <vector>

void setup() {
    std::vector<std::unique_ptr<EquipmentPiece>> pieces;
    pieces.push_back(std::make_unique<EquipmentPiece>(1001));
    pieces.push_back(std::make_unique<EquipmentPiece>(1002));
}
```

## 小结

- 缺省构造合理时保留（数字语义、空容器）；**无外部数据则对象无意义时，不要提供**。
- 无缺省构造的限制：对象数组、部分模板、虚基类传参——可用指针容器、`emplace_back` 等替代，而非引入「未确定」状态。
- **构造保证有效** 优于 **默认构造 + 处处有效性检测**：更简单、更快、 invariant 更强。
