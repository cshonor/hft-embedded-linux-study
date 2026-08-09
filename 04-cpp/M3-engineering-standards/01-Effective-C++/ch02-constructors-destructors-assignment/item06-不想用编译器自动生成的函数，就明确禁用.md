# 条款 6：不想用编译器自动生成的函数，就明确禁用

## 本节讲什么

**Explicitly disallow the use of compiler-generated functions you do not want.** [条款 5](./item05-了解C++默默编写并调用哪些函数.md) 说编译器会生成 public 的拷贝构造 / 拷贝赋值；对**独一无二**的对象（如待售房产 `HomeForSale`、文件句柄、单例），拷贝应**明确禁止**。

---

## 为什么要禁拷贝

```cpp
class HomeForSale { /* 每套房唯一 */ };

HomeForSale h1;
HomeForSale h2(h1);      // 若允许 → 语义荒谬
HomeForSale h3;
h3 = h1;                 // 同上
```

不声明时，编译器会生成 **public** 的 copy ctor / `operator=` → 必须主动拦截。

---

## 1. 声明为 `private` 且不提供实现（经典写法）

### 步骤

1. **自己声明** copy ctor 和 `operator=` 为 **`private`** —— 阻止编译器生成 public 缺省版，也阻止外部客户调用。
2. **故意不写出函数体（不定义）** —— 成员 / 友元若误调用，**链接期**找不到定义 → 报错。

```cpp
class HomeForSale {
public:
    HomeForSale() = default;
    // ...
private:
    HomeForSale(const HomeForSale&);             // 只声明
    HomeForSale& operator=(const HomeForSale&);  // 只声明
    // 不提供 { ... } 定义
};
```

| 谁调用 | 结果 |
|--------|------|
| 外部代码 | **编译错误**（private） |
| 成员 / 友元误拷贝 | **链接错误**（无定义） |

**iostream** 等标准库类型常用此手法禁止拷贝（C++11 前）。

### 局限

- 错误发现可能拖到 **link-time**；
- 每个要禁拷贝的类都要重复写两行 private 声明。

---

## 2. 继承 `Uncopyable` 基类（编译期拦截）

把防线提前到 **compile-time**：抽一个**专门禁止拷贝**的基类。

```cpp
class Uncopyable {
protected:
    Uncopyable() = default;
    ~Uncopyable() = default;
private:
    Uncopyable(const Uncopyable&);
    Uncopyable& operator=(const Uncopyable&);
};

class HomeForSale : private Uncopyable {
public:
    HomeForSale() = default;
    // 未声明拷贝 — 编译器尝试生成 HomeForSale 的拷贝函数时
    // 必须调用 Uncopyable 的拷贝，而基类拷贝是 private → 编译失败
};
```

- **`private` 继承**：不是「是一个 Uncopyable」，只是**实现技巧**混入禁拷贝能力。
- 成员 / 友元试图拷贝 → **编译器直接拒绝**（比链接期更早）。

**Boost**：可直接用 **`boost::noncopyable`**（同思路；C++11 后更常直接用 `= delete`）。

---

## 3. 现代写法：` = delete`（C++11 起，首选）

今天最清晰的做法：**public 声明 + `= delete`**，编译期报错且意图一目了然。

```cpp
class HomeForSale {
public:
    HomeForSale() = default;

    HomeForSale(const HomeForSale&) = delete;
    HomeForSale& operator=(const HomeForSale&) = delete;
};
```

| 方式 | 报错时机 | 可读性 |
|------|----------|--------|
| private + 无定义 | 外部 compile / 成员 link | 中 |
| Uncopyable 继承 | compile | 中（需基类） |
| **`= delete`** | **compile** | **高** |

`delete` 也可用于**其它**不想提供的重载（如 taking `double` 的 `operator=` 禁掉隐式转换赋值）。

```cpp
class BigInt {
public:
    BigInt& operator=(const BigInt&) = default;
    BigInt& operator=(double) = delete;  // 禁止 double 赋值
};
```

---

## 与条款 5 的关系

| 条款 5 | 条款 6 |
|--------|--------|
| 编译器**会**生成什么 | **不要**的生成物如何**拒绝** |
| 引用/const 导致无法生成 `operator=` | 语义上就不该拷贝 → **主动 delete / private** |

资源类（Rule of Three / Five）：要么**正确实现**拷贝/移动，要么 **`= delete`** 拷贝（只移动或不可复制）。

---

## HFT 视角

适合 **= delete** 拷贝的类型：

- **单例 / 全局配置 accessor** 返回的句柄包装；
- **socket、fd、mmap 区域** 的唯一所有权；
- **线程、`std::thread`** 式不可拷贝工作者；
- **订单簿里「席位 / 连接」** 代表独占资源的对象。

可拷贝的应是 **值语义** 数据（价格、数量、tick）；不可拷贝的应是 **身份 / 句柄**。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 以为不声明就不会拷贝 | 编译器会生成 **public** 拷贝 |
| 只 private 不定义，成员误拷贝 | link 才报错 → 优先 `= delete` 或 Uncopyable |
| 禁拷贝却忘了移动 | C++11 起显式 `= default` move 或一并 delete |
| 多态基类只 delete 拷贝 | 仍要 [条款 7](./item07-多态基类声明virtual析构函数.md) **virtual 析构** |

---

## 核心总结

1. 不想用的编译器函数 → **明确拒绝**，别依赖「我不写就没有」。
2. **经典**：private 声明 + **不定义**（link-time 抓成员/友元）。
3. **更好**：**private 继承 Uncopyable** / Boost `noncopyable`（compile-time）。
4. **现代首选**：**`= delete`** copy ctor / copy assign（compile-time + 意图清晰）。
5. 独一无二、独占资源 → **禁拷贝**；值类型 → 实现或默认 member-wise。

← [条款 5：编译器生成什么](./item05-了解C++默默编写并调用哪些函数.md) | [条款 7：virtual 析构 →](./item07-多态基类声明virtual析构函数.md)
