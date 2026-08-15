# Item 1 · 06 设计模式与重构

← [Item 1 目录](./README.md)

Effective Rust Item 1 的核心主张：**用类型表达设计意图，让无效状态无法被类型表达**（*Make invalid states inexpressible*）。

本章不是讲 GoF 那套「23 种设计模式」，而是讲 **Rust 里用类型系统做设计**——不靠类和继承，靠 struct / enum / trait 在编译期把非法组合挡在外面。

## 重点结论

1. **用类型表达设计意图**——对人、对编译器都清晰。
2. **Make invalid states inexpressible**——合法组合才写得出来；非法组合**根本写不进类型**。
3. **缺失值 / 失败**——分别用 `Option` / `Result`，见 [05-option-result.md](./05-option-result.md)。

---

## 「让无效状态无法被类型表达」是什么意思？

一句话：**把「只能处于其中一种状态」这件事写进类型，而不是靠注释、运行时检查或 `Option` 字段凑出来。**

### 高频交易订单：struct 反模式

用「一个大 struct + 一堆可选字段」存订单，很容易留下**逻辑上不可能同时成立**的组合：

```rust
struct Order {
    create_time: Option<u64>,   // 挂单时间
    trade_time: Option<u64>,    // 成交时间
    price: Option<i64>,         // 成交价格
    cancel_time: Option<u64>,   // 撤单时间
}
```

问题在于：

| 实际业务 | struct 却允许 |
|----------|---------------|
| 未成交订单只有 `create_time` | `create_time` 和 `trade_time` 同时 `Some` |
| 已成交订单有 `trade_time` + `price` | 已成交还带 `create_time` |
| 已撤单只有 `cancel_time` | 撤单了还留着 `price` |

编译器**不会**因为你填了矛盾字段而报错——只能靠测试、Code Review 或线上 bug 去发现。对容不得错的高频交易系统，这种 bug 代价极高。

### 同一业务：用 enum 从根上消灭

```rust
enum OrderStatus {
    Pending { create_time: u64 },
    Filled { trade_time: u64, price: i64 },
    Cancelled { cancel_time: u64 },
}
```

每个变体**只携带该状态下合法的字段**：

- `Pending` → 必有 `create_time`，不可能带 `trade_time` / `price`
- `Filled` → 必有 `trade_time` 和 `price`，不可能还带挂单语义
- `Cancelled` → 必有 `cancel_time`

`match order_status { ... }` 时编译器要求**穷尽所有变体**，新增状态必须改 enum，漏分支会编译失败——这就是「无效状态无法被类型表达」。

> 与 [04-enum-adt.md](./04-enum-adt.md) 里的 `OrderState` 示例同一思路；这里强调**为什么要从 struct 迁到 enum**。

---

## 反模式 → 用 enum 消灭非法组合

**重构前**（靠注释约定，字段可处于非法组合）：

```rust
struct ScreenColor {
    monochrome: bool,
    fg_color: RgbColor, // 注释：monochrome 为 true 时 fg 应为 0
}
```

**重构后**（无效组合在类型层面不存在）：

```rust
enum Color {
    Monochrome,
    Foreground(RgbColor),
}
struct ScreenColor {
    color: Color,
}
```

---

## 核心套路：互斥字段 → 合成一个 enum 字段

你最后那句总结**完全正确**。可以记成三步：

```text
① 发现 struct 里若干字段「不能同时成立 / 只能选一种」
② 把这些字段抽出来，做成 enum 的互斥变体
③ struct 里删掉原字段，只留一个 enum 类型的字段
```

### 屏幕颜色：从两个字段变成一个字段

**重构前**——两个字段可能打架：

```rust
struct ScreenColor {
    monochrome: bool,      // 是否单色
    fg_color: RgbColor,    // 彩色值（注释说：单色时应为 0）
}
// ❌ 非法但写得出来：monochrome = true 且 fg_color 有彩色值
```

`monochrome` 和 `fg_color` **互斥**：单色模式不该有有效彩色值，彩色模式又不该标成单色。它们不是两个独立开关，而是**同一维度的两种状态**。

**重构后**——合成一个 `Color` enum，struct 只留一个字段：

```rust
enum Color {
    Monochrome,              // 变体 1：单色，不带 RGB
    Foreground(RgbColor),    // 变体 2：彩色，必须带 RGB
}

struct ScreenColor {
    color: Color,   // ✅ 只有一个字段，当前只能是两种之一
}
```

| 原来 | 现在 |
|------|------|
| `monochrome: bool` + `fg_color: RgbColor` 两个字段 | `color: Color` **一个**字段 |
| 可能「单色 + 有彩色值」 | `Monochrome` 变体**根本没有** RGB 字段 |
| 靠注释约定 | 靠类型**写不出来**非法组合 |

订单 `OrderStatus` 同理：四个 `Option` 字段 → **一个** `status: OrderStatus` 枚举字段。

---

## 布尔参数 → 具名枚举（像「带校验的下拉框」）

标题拆开读：**布尔参数** = 用 `true`/`false` 传选项；**具名枚举** = 给每个选项起**有意义的名字**，放进 `enum` 里。

### 枚举（enum）是什么？

先搞懂「枚举」本身（语法细节见 [04-enum-adt.md](./04-enum-adt.md)）：

- **枚举** = 一个值**只能是几种选项之一**（互斥），比如「单面 / 双面」「黑白 / 彩色」。
- 每个选项叫**变体**（variant），如 `Sides::One`、`Output::Color`。
- 和 `bool` 只有 `true`/`false` 两个匿名值不同，变体**自带名字**，读代码的人一眼知道在选什么。

一句话：**enum 把「只能选其一的几种情况」写进类型**。

### 类比：带类型校验的下拉列表

| 概念 | 对应 |
|------|------|
| 下拉框里的选项 | enum 的变体（`Sides::One`、`Output::Color`） |
| 用户选其中一项 | 传参时写 `Sides::Both` 或 `Output::BlackAndWhite` |
| 只能选一个 | 枚举**互斥**——不能同时是 `One` 和 `Both` |
| 类型校验 | `Sides` 和 `Output` 是**不同类型**，塞错参数位编译报错 |

布尔参数像只有两个没标签的按钮（`true`/`false`）；具名枚举像**每个选项都有名字的下拉框**，而且不同下拉框类型不同，选错框都过不去编译。

### 问题：`print_page(true, false)` 在说什么？

```rust
// ❌ 布尔参数：意图模糊，顺序还容易传反
print_page(true, false);
//        ^^^^  ^^^^^ 第一个 true 是单面还是双面？
//               ^^^^^ 第二个 false 是黑白还是彩色？
```

`true`/`false` 本身**没有业务含义**——只能靠参数位置、注释或文档猜。两个 `bool` 参数还可以**传反**而编译器不报错（都是 `bool`，类型一样）。

### 重构：给选项「上户口」

```rust
enum Sides {
    One,   // 单面
    Both,  // 双面
}

enum Output {
    BlackAndWhite,
    Color,
}

fn print_page(sides: Sides, output: Output) { /* ... */ }

// ✅ 调用处意图一眼能看懂
print_page(Sides::One, Output::BlackAndWhite);
```

| 对比 | 布尔参数 | 具名枚举 |
|------|----------|----------|
| 可读性 | `true, false` 看不出含义 | `Sides::One, Output::Color` 自解释 |
| 传参顺序 | 两个 `bool` 传反也编译通过 | `Sides` 和 `Output` **不同类型**，传反会编译错误 |
| 扩展 | 再加选项只能堆更多 `bool`，组合爆炸 | 新改变体，编译器逼你补全 `match` |

**具名枚举** = 不再用裸 `true`/`false`，而是给每个选项一个**有名字、有类型**的身份。

---

## 前后状态有依赖时怎么设计？

业务里常见：**前一个状态决定后面能做什么**（未支付不能发货、已成交不能再挂单）。设计目标：

1. **每个状态只带该状态下合法的字段**（见上文 `OrderStatus`）。
2. **每个状态只开放合法操作**——非法操作在类型或 `match` 里直接拦住。
3. **状态转换显式**——从 A 到 B 是一次明确的转换，不是改几个字段。

### 订单：未支付 → 已支付 → 已发货

```rust
enum Order {
    Unpaid { amount: u64 },
    Paid { amount: u64, paid_at: u64 },
    Shipped { amount: u64, paid_at: u64, tracking: String },
}
```

- `Unpaid`：只有金额，**没有**物流单号——类型上就不存在「未支付却有 tracking」。
- 支付成功：`Unpaid` **变成** `Paid`（新变体，带上 `paid_at`）。
- 发货：只有 `Paid` 才能变成 `Shipped`；`Unpaid` 走发货路径应在 `match` 里返回错误。

前序状态对后续的影响，通过**变体转换**写清楚，而不是在一个 struct 里改 flag。

---

## 不同状态对应不同功能：怎么做到？

常见两种写法；**Item 1 阶段优先掌握 enum + `match`**（逻辑集中、一眼看清差异）。

### 写法一：enum 上的方法 + `match`（推荐入门）

把「当前状态能做什么」写进 `match` 各分支；**不合法的分支直接 `Err` 或编译期就写不出来**：

```rust
enum Color {
    Monochrome,
    Foreground(RgbColor),
}

impl Color {
    fn draw(&self) {
        match self {
            Color::Monochrome => { /* 只走灰度渲染 */ }
            Color::Foreground(rgb) => { /* 走彩色渲染，用 rgb */ }
        }
    }
}
```

订单支付同理——方法消费 `self`，成功则**返回新状态的 enum 值**：

```rust
impl Order {
    fn pay(self) -> Result<Order, &'static str> {
        match self {
            Order::Unpaid { amount } => Ok(Order::Paid {
                amount,
                paid_at: now(),
            }),
            Order::Paid { .. } => Err("already paid"),
            Order::Shipped { .. } => Err("already shipped"),
        }
    }

    fn ship(self) -> Result<Order, &'static str> {
        match self {
            Order::Paid { amount, paid_at } => Ok(Order::Shipped {
                amount,
                paid_at,
                tracking: assign_tracking(),
            }),
            Order::Unpaid { .. } => Err("must pay first"),
            Order::Shipped { .. } => Err("already shipped"),
        }
    }
}
```

- 持有一个 `Order::Unpaid { ... }` 时，调用 `pay()` 得到 `Order::Paid`；**类型变了，能调的方法语义也跟着变**。
- 在 `Unpaid` 上误调 `ship()` → 运行时 `Err`；若把状态拆成不同 struct + 不同 impl（类型状态模式，Item 进阶），部分非法调用可提升到**编译期**报错。

### 写法二：trait 对象（了解即可，Item 2 再深入）

给每个状态单独 `impl OrderState trait`，用 `Box<dyn OrderState>` 动态分发。灵活，但样板多、间接层也多；**同等逻辑用 enum + `match` 往往更直观**。见 [Item 2](../Item-02-express-common-behavior/README.md) trait 专题。

---

## 相关

- ADT 语法 → [04-enum-adt.md](./04-enum-adt.md)
- 易错细节 → [07-pitfalls.md](./07-pitfalls.md)
