# Item 1: Use the type system to express your data structures

> **Effective Rust** · [Chapter 1 — Types](../../ER-本书目录.md)  
> **中文**：使用类型系统表达数据结构  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [ ] demo（见 [ER-demos 索引](../../ER-demos/README.md) / Book demo）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| 标量、数组、元组 | [3.2 数据类型](../../00-Book/03-common-concepts/3.2-数据类型.md) |
| 结构体 | [5.1 结构体](../../00-Book/05-structs/5.1-定义并实例化结构体.md) |
| 枚举、ADT、`Option` | [6.1 定义枚举](../../00-Book/06-enums-pattern-matching/6.1-定义枚举.md) |
| `Result`、错误处理 | [9.2 Result](../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) |

---

## 一句话

**用类型表达设计意图，让无效状态在编译期写不出来**（Make invalid states inexpressible）。

---

## 专项笔记（按需点开）

| 专题 | 内容 | 阅读 |
|------|------|------|
| 01 | 整数、无隐式转换 | [01-fundamental-types.md](./01-fundamental-types.md) |
| 02 | `bool` / 浮点 / `()` / `char` | [02-scalar-types.md](./02-scalar-types.md) |
| 03 | 数组、元组、结构体 | [03-aggregate-types.md](./03-aggregate-types.md) |
| 04 | 枚举与 ADT | [04-enum-adt.md](./04-enum-adt.md) |
| 05 | `Option` / `Result` | [05-option-result.md](./05-option-result.md) |
| 06 | 设计模式与重构案例 | [06-design-patterns.md](./06-design-patterns.md) |
| 07 | 易错细节 | [07-pitfalls.md](./07-pitfalls.md) |
| — | 背诵提纲（考前速览） |

---

## 逻辑脉络

```text
严格基础类型 → 静态布局（struct / tuple）
      → 带数据的 enum 表达业务状态
      → match 穷尽性检查
      → Option / Result + ? 统一错误传播
```

- **类型安全递进**：编译期捕获无效状态，而非运行时兜底。
- **穷尽性 `match`**：必须处理每个变体，否则不编译。
- **与 Item 3、4 衔接**：转换方法 + `?` + 错误类型设计。

---

## 后续拓展

> [ER-拓展索引 § Item 01](../../ER-拓展索引.md#item-01)

---

## 速记

考前 / 面试前 3 分钟速览。

## 四句话总览（默写版）

1. **基础类型**：位数明确、强类型、整数 / `char` **不隐式转换**。
2. **聚合**：数组同构定长，元组异构定长，结构体命名字段。
3. **enum**：互斥变体 + 可带数据 = **ADT**。
4. **Option / Result**：无 null；错误必须显式处理。

## 速查表

| 块 | 背一句 |
|----|--------|
| 整数 | `i32`/`u64` 等**定长**；非 Python 自动扩容 int |
| 布局 | 固定大小 → 结构体紧凑、缓存友好（HFT） |
| 比较 | 异类型不能 `==`；先 `as` / `From` **间接比较** |
| 溢出 | debug panic；release 默认环绕 → 用 `checked_*` / `wrapping_*` 显式定语义 |
| 标量 | `bool` · `f32/f64` · `()` · `char`（4B Unicode 标量） |
| `bool` | 1 字节；`true`≈`0x01`、`false`≈`0x00`；≠ 全 1 / 全 0 |
| `()` | 无可用返回值；无返回类型或语句结尾 `;` → 返回 `()` |
| 数组 | `[T; N]` 定长同构；`arr[i]` 下标，越界 panic |
| 元组 | 定长异构；`.0` `.1`（编译期字段） |
| struct | 命名字段；元组结构体 `.0` + 类型名约定（RGB、Price） |
| `mut` | **绑定**级可变，非「值类型/引用类型」默认规则 |
| ADT | **积** struct/tuple · **和** enum · `()` = 零元积 |
| `usize`/`isize` | 指针同宽；`len`/索引；业务字段别滥用 |
| `Option` | `Some` / `None` → 无 null；管**存在性**（不是成败） |
| `Result` | `Ok` / `Err` → 管**成败**；断网/超时走 `Err`，不是 `None` |
| `Result<Option<Vec>>` | 先 `Result` 再 `Option` 再 `Vec`；三层别混 |

## Item 1 设计主张

| 要点 | 一句 |
|------|------|
| 核心主张 | 设计写进类型，无效状态编译不过 |
| 无效状态 | struct+`Option` 字段易留非法组合 → 用 enum 变体各带合法字段 |
| 订单例 | `Pending`/`Filled`/`Cancelled` 各带自己的 time/price，不能混搭 |
| 布尔→枚举 | 像带校验的下拉框；`Sides::One` 不能塞进 `Output` 参数位 |
| 互斥字段 | struct 里打架的多个字段 → 合成一个 `enum` 字段（如 `color: Color`） |
| 状态依赖 | 前态影响后态 → enum 变体转换 + 各分支只开放合法操作 |
| 状态功能 | 优先 `impl Enum { match self { ... } }`，trait 对象作进阶 |
| ADT | `enum` 变体可带数据 |
| 哨兵 | 用 `Option`，别用魔法值 |
| 失败 | 用 `Result`，别用特殊返回码 |
| `Error` | 自定义 `E` 惯例实现 `Error`；库用 `thiserror`（→ Item 4） |
| `match` | 穷尽；公开 enum 加变体要小心破坏性 |

## 专题索引

| # | 文件 |
|---|------|
| 01 | [fundamental-types](./01-fundamental-types.md) |
| 02 | [scalar-types](./02-scalar-types.md) |
| 03 | [aggregate-types](./03-aggregate-types.md) |
| 04 | [enum-adt](./04-enum-adt.md) |
| 05 | [option-result](./05-option-result.md) |
| 06 | [design-patterns](./06-design-patterns.md) |
| 07 | [pitfalls](./07-pitfalls.md) |

