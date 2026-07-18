# 条款 12：复制对象时勿忘其每一个成员

## 本节讲什么

**Copy all parts of an object.** 良好 OOP 里，**拷贝构造函数**与**拷贝赋值运算符**（合称 **拷贝函数**）负责复制对象。编译器默认版会 **member-wise** 拷贝全部数据（[条款 5](./item05-了解C++默默编写并调用哪些函数.md)）；**一旦自写**，就必须拷贝 **每一个数据成员 + 每一个基类子对象**，且 **禁止用一个拷贝函数实现另一个**。

← [条款 11：自我赋值](./item11-赋值运算符处理自我赋值.md) · 第二章收尾 → [条款 13：RAII](../ch03-resource-management/item13-以对象管理资源（RAII）.md)

---

## 拷贝函数的责任范围

```
完整拷贝 = 基类部分 + 本类所有非 static 数据成员
```

漏拷任一字段 → **部分拷贝**；编译器**不会警告**（你已接管拷贝逻辑）。

---

## 1. 新增成员 → 同步更新所有拷贝相关函数

为类增加数据成员后，必须检查并更新：

- 拷贝构造函数  
- 拷贝赋值运算符（及 [条款 10](./item10-令operator=返回this引用.md) 的 `return *this`、[条款 11](./item11-赋值运算符处理自我赋值.md) 的安全写法）  
- **所有构造函数**（若新成员需特殊初始化）  
- 任何非标准形式的 `operator=`  

```cpp
class Widget {
    int id;
    std::string name;
    // 新增：
    double weight;   // ← 若 copy ctor/operator= 未拷贝 weight → 静默 bug
};
```

**习惯**：改类布局时，把 copy ctor / copy assign / 析构 当作 **Rule of Three** 一组一起改（C++11 起扩展为 Rule of Five：+ move ctor / move assign）。

---

## 2. 派生类：必须拷贝基类部分

继承下只拷派生成员、忘记基类 → **基类子对象错误初始化或未更新**。

### 拷贝构造函数：成员初始化列表调用基类 copy ctor

```cpp
class Base { /* ... */ };
class Derived : public Base {
    int derivedData;
public:
    Derived(const Derived& rhs)
        : Base(rhs)              // ✅ 必须：拷贝基类部分
        , derivedData(rhs.derivedData)
    {}

    // ❌ 若省略 Base(rhs) → 只调 Base 的 default ctor，基类成员未从 rhs 拷贝
};
```

基类成员多为 **private**，派生类**不能**直接访问 → **只能**通过基类拷贝函数。

### 拷贝赋值：显式调用基类 `operator=`

```cpp
Derived& Derived::operator=(const Derived& rhs) {
    if (this == &rhs) return *this;
    Base::operator=(rhs);        // ✅ 先赋基类部分
    derivedData = rhs.derivedData;
    return *this;
}
```

省略 `Base::operator=(rhs)` → 基类子对象**保持旧值**，只有派生部分被更新。

| 函数 | 基类部分 |
|------|----------|
| **拷贝构造** | 初始化列表 **`Base(rhs)`** |
| **拷贝赋值** | **`Base::operator=(rhs)`**（通常放在最前） |

---

## 3. 禁止：用一个拷贝函数调用另一个

copy ctor 与 `operator=` 代码常相似，但**语义不同**，不能互相调用：

| 错误尝试 | 为何荒谬 |
|----------|----------|
| 在 **`operator=` 里调 copy ctor** | 对象**已存在**，不能对已存在对象再「构造」 |
| 在 **copy ctor 里调 `operator=`** | 构造阶段对象**尚未完全初始化**，赋值无意义 |

C++ **没有**「对已存在对象重新运行构造函数」的合法语法来复用 copy ctor。

### 正确做法：`private` 辅助函数（常名 `init`）

```cpp
class Widget {
public:
    Widget(const Widget& rhs) { initFrom(rhs); }
    Widget& operator=(const Widget& rhs) {
        if (this == &rhs) return *this;
        initFrom(rhs);
        return *this;
    }
private:
    void initFrom(const Widget& rhs) {
        // 共用的深拷贝 / 成员赋值逻辑
    }
};
```

- **copy ctor**：构造完成后或列表初始化 + `initFrom` 填成员  
- **`operator=`**：先处理资源/自赋值（条款 11），再 `initFrom` 或 copy-and-swap  

与 [条款 9](./item09-绝不在构造和析构过程调用虚函数.md) 不同：这里的 `initFrom` 是 **non-virtual 私有辅助**，且不在基类 ctor 里做依赖派生状态的多态。

---

## 与条款 11 的配合

手写 `operator=` 时通常：

1. [条款 12] 拷 **基类 + 全部成员**  
2. [条款 11] **自赋值 + 异常安全**（先 new 后 delete / copy-and-swap）  
3. [条款 10] **`return *this`**

```cpp
Derived& Derived::operator=(const Derived& rhs) {
    if (this == &rhs) return *this;
    Derived temp(rhs);           // copy-and-swap：temp 已 copy 全部分（含基类）
    swap(temp);
    return *this;
}
```

`Derived temp(rhs)` 依赖 **copy ctor 正确拷贝所有 part**。

---

## HFT 视角

- **订单 / 持仓结构体** 增字段（如 `client_id`）时，自定义 copy 的撮合引擎缓存会 **部分拷贝** → 静默错单；改 struct 必查 Rule of Three/Five。  
- **策略派生类** 拷贝时漏 `Base::operator=` → 基类里 **风控参数** 不更新。  
- 优先 **`= default`** / 编译器生成（成员皆可正确拷贝时），减少手写漏字段；资源成员用 [条款 13](../ch03-resource-management/item13-以对象管理资源（RAII）.md) 智能指针。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 新加成员未更新 copy 函数 | 改类即查 copy ctor / assign / dtor |
| `Derived(const Derived&)` 未写 `Base(rhs)` | 初始化列表显式调基类 copy ctor |
| `Derived::operator=` 未调 `Base::operator=` | 第一句或 copy-and-swap 的 temp 构造 |
| copy ctor 里调 `operator=` | 抽 `initFrom` 共用 |
| 只拷派生 `private` 能见的成员 | 基类 private → 只能基类 copy 函数 |

---

## 核心总结

1. 自写拷贝函数 → 拷 **全部成员 + 全部基类子对象**。  
2. **改类结构** → 同步更新所有拷贝函数与相关 ctor。  
3. 派生 **copy ctor**：**`Base(rhs)`**；派生 **`operator=`**：**`Base::operator=(rhs)`**。  
4. **不要** copy ctor ↔ `operator=` 互调 → **`private initFrom`**（或 copy-and-swap 统一路径）。  
5. 编译器**不提醒**漏字段 —— 这是手写拷贝的代价；能 `= default` 则 default。

← [条款 11：自我赋值](./item11-赋值运算符处理自我赋值.md) | [第三章：条款 13 RAII →](../ch03-resource-management/item13-以对象管理资源（RAII）.md)
