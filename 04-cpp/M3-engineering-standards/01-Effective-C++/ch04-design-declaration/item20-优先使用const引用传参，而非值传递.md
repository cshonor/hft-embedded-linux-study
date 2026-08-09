# 条款 20：优先使用 const 引用传参，而非值传递

## 本节讲什么

**Prefer pass-by-reference-to-const over pass-by-value.** C 遗产的**传值**会对参数做**拷贝构造**、出作用域时**析构**——复杂类型代价极高；传 **`const T&`** 无额外构造、**不修改实参**，且**避免对象切断（slicing）**。例外：**内置类型**、STL **迭代器**与**函数对象**通常仍传值。

← [条款 19：类即新类型](./item19-设计类等同于设计一种全新类型.md) · [条款 21：按值返回对象 →](./item21-必须返回对象时，不要强行返回引用.md)

---

## 1. 传值的昂贵代价

缺省习惯（C 遗产）：参数按**值**传入 → 用**拷贝构造函数**初始化形参；函数结束形参**析构**。

```cpp
void f(Student s);   // 调用一次 copy ctor 构造 s；返回时 dtor
```

复杂对象会**连锁**触发多次构造/析构：

```
派生类对象（含多个 string、继承基类）
    → 基类子对象 copy
    → 每个 string copy（可能堆分配）
    → 派生类特有成员 copy
    → 函数结束：上述全部逆序 dtor
```

| 因素 | 传值成本 |
|------|----------|
| 多层继承 | 基类 + 派生部分分别拷贝 |
| 多个 `string` / 容器成员 | 深拷贝、分配 |
| 频繁调用的小函数 | 拷贝成本 × 调用次数 |

C++11 起 **move** 可减轻「从右值传值」的开销，但**从左值传值**仍通常拷贝；且 move 后源对象状态受限——**只读形参**仍首选 `const T&`。

---

## 2. 传引用给 const 的优势

```cpp
void print(const std::string& s);   // ✅ 推荐
// void print(std::string s);       // ❌ 每次调用可能拷贝整串
```

### 消除不必要构造/析构

`const T&` **不创建新对象**（仅绑定到实参），无额外 copy/move ctor、无参数 dtor → 大对象、热路径函数收益明显。

### `const` = 防篡改保证

| 方式 | 函数内修改实参？ | 客户感知 |
|------|------------------|----------|
| **传值** | 改的是**副本**，不影响实参 | 安全，但付了拷贝费 |
| **`T&`** | 可能改实参 | 需文档说明 |
| **`const T&`** | **不能**改实参 | 与传值同等「只读」保证，**无拷贝** |

传引用时**务必加 `const`**，除非接口本意就是修改实参（则用 `T&`）。

### 避免切断（Object Slicing）

```cpp
class Person { /* ... */ virtual std::string name() const; };
class Student : public Person { /* ... */ };

void study(Person p);              // ❌ 传值：只拷贝 Person 子对象
void study(const Person& p);       // ✅ 保留完整 Student

Student st;
study(st);   // 传值 → 函数里 p 只是 Person，多态丢失
             // 传 const 引用 → 动态类型仍是 Student
```

传值进「接受基类」的形参时，**派生部分被切掉**；即便有 `virtual`，对象已是**纯基类子对象**。  
**多态形参几乎总是 `const Base&` 或 `Base&`（或指针）**（见 [条款 7](../ch02-constructors-destructors-assignment/item07-为多态基类声明virtual析构函数.md)）。

```
Student 对象 ──传值──→ [Person 子对象副本]  ← 派生部分丢弃
Student 对象 ──const&──→ 完整 Student（动态类型保留）
```

---

## 3. 何时仍用传值

| 类型 | 倾向传值 | 原因 |
|------|----------|------|
| **`int`、`double`、`bool`、指针** 等内置类型 | ✅ | 尺寸固定、拷贝 ≈ 几个机器字；有时进寄存器 |
| **STL 迭代器** | ✅ | 标准为值语义设计，常似指针大小 |
| **函数对象（functor）** | ✅ | 通常小而可拷贝；算法按值传递策略对象 |

```cpp
void setFlag(bool on);                                    // 传值
template<typename Iter, typename Pred>
void algo(Iter first, Iter last, Pred pred);              // Pred 常传值
```

**不要**把「自定义类型」默认当成内置类型——见下节「小对象陷阱」。

---

## 4. 警惕「小对象」陷阱

「类里只有一个指针，sizeof 很小 → 传值便宜」**不可靠**：

| 误解 | 现实 |
|------|------|
| 小 = 拷贝便宜 | 指针成员可能触发**深拷贝**（堆、锁、文件） |
| 与 `double` 一样大就该传值 | 编译器可能愿把 **`double` 放寄存器**，却不愿对 **含 `double` 的 class** 同样优化 |
| 现在小永远小 | 实现演进后成员变多，**传值接口**会悄悄变慢 |

```cpp
class Handle {
    int* p;   // 看似 8 字节
public:
    Handle(const Handle& other);  // 可能深拷贝 *p 指向的资源
};
void f(Handle h);   // ❌ 可能极贵
void f(const Handle& h);  // ✅
```

**惯例：自定义类型形参默认 `const T&`**；仅 profiling 或明确值语义（如 `std::optional` 小对象、移动 sink）再传值。

---

## 与条款 19、21 的关系

| 条款 | 关系 |
|------|------|
| **19** | 「按值传递意味着什么？」→ copy/move 语义；本条规定**接口默认**怎么传 |
| **21** | 本条款管**入参**；返回局部新对象应**按值返回**（RVO/NRVO），不要为省拷贝返回悬垂引用 |

---

## HFT 视角

- **Order / Quote / BookLevel**：含 `string`、`vector` 或共享状态 → **`const Order&`**，禁止热路径 `void onOrder(Order o)`。  
- **回调** `void onTick(const Tick& t)`：const 引用 + 短生命周期；若跨线程队列再**显式拷贝**到队列内。  
- **策略 functor**：传给 `std::for_each` 等可传值；**行情结构体**不是 functor，别套用。  
- Code review：**多态基类形参是否传值**（slicing 一票否决）。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `void f(string s)` | `void f(const string& s)` |
| `void process(Base b)` | `void process(const Base& b)` |
| 传引用只读却忘 `const` | `const T&` |
| 小 class 一律传值 | 看 copy ctor 成本，不只看 sizeof |
| 内置类型强行 `const int&` | 通常**传值**更简、有时更快 |

---

## 核心总结

1. **自定义类型入参**：默认 **`const T&`**，避免拷贝构造 + 析构链。  
2. **`const`** 在传引用时提供与传值同等的**只读**保证。  
3. **传值进基类形参** → **slicing**，多态失效 → 用 **`const Base&`**。  
4. **例外**：内置类型、迭代器、函数对象 → **传值**通常更合适。  
5. **小对象 ≠ 廉价拷贝**；类型实现会变——接口选 `const&` 更稳。

← [条款 19：类即新类型](./item19-设计类等同于设计一种全新类型.md) | [条款 21：按值返回对象 →](./item21-必须返回对象时，不要强行返回引用.md)
