# 1.2.1 · 通用标准 Trait 完整解读（Unsurprising）

← [02 通用 Trait](./02-common-traits-for-types.md)

> **准则**：对外公开类型尽量**开箱即用**，实现用户预期的基础 Trait。  
> **定位**：下文三类是**库作者最佳实践**，不是语法强制；内部业务可灵活，但 **impl 标准 Trait 时调用方预期不变**。  
> ER → [Item 10 标准 trait](../../01-ER/Chapter-02-Traits/Item-10-standard-traits/README.md) · Book → [10.2 trait · derive](../../00-Book/10-generics-traits-lifetimes/10.2-trait.md)

---

## 标准 Trait vs 自定义 Trait

| | 标准 Trait（std） | 自定义 Trait |
|---|-------------------|--------------|
| **例子** | `Debug` · `Send` · `Copy` · `Hash` | `Strategy` · `MarketDataSource` |
| **是否适用三类** | ✅ 给 `Order` 等类型 impl 时遵守 | ❌ 不硬套Ⅰ/Ⅱ/Ⅲ；归 **按需设计** |
| **公开给别人用时** | 违反三类 → 用户 `Debug`/`spawn` 踩坑 | 接口命名、对象安全、文档仍要 Unsurprising |
| **纯内部业务** | 可省略 `Debug` 等，编译不拦 | 团队约定即可 |

```rust
// 自定义 trait：按需，不属于「几乎总实现」那类
trait Strategy {
    fn on_tick(&mut self, price: f64);
}

// 标准 trait：Order 若对外发布，仍建议按三类
#[derive(Debug, PartialEq, Eq)]
struct Order { id: u64, symbol: String }
// Send：字段全 Send 则自动满足 — 策略线程间传 Order 靠这个
```

三类划分只组织 **「公开类型 × 标准 Trait」**；`Strategy` 怎么设计见 [03 人体工程学 impl](./03-ergonomic-trait-implementations.md)。

---

## 划分标准：三类边界

| 维度 | Ⅰ 几乎总实现 | Ⅱ 线程默认契约 | Ⅲ 谨慎对待 |
|------|-------------|----------------|------------|
| **代表 Trait** | `Debug` · `PartialEq` / `Eq` | `Send` · `Sync` | `Copy` · `Hash` |
| **影响范围** | 只增**调试 / 相等**便利 | 决定能否**跨线程**传递共享 | 锁死**内存模型**或 **map 不变式** |
| **副作用** | 几乎无；不改变核心行为 | 编译器**自动推断**为主 | Copy 隐式复制；Hash 绑定相等语义 |
| **违反代价** | 不加 → 调试卡壳、测试写不顺 | 非 Send → `spawn` 编译不过 | Copy 后难改结构；Hash≠Eq → map 丢键 |
| **策略** | 公开类型**默认 derive** | 默认满足；例外**文档说明** | **按需**、慎重、成对使用 |

### 派生宏 ↔ 风险等级

| Trait | `derive` 默认 | 含义 |
|-------|:-------------:|------|
| `Debug` | ✅ 常加 | 低风险，随时可补 |
| `PartialEq` / `Eq` | ✅ 常加 | 低风险 |
| `Send` / `Sync` | ✅ 自动实现（安全组合） | 不满足才需特别处理 |
| `Copy` | ❌ **须显式** | 高风险，compiler 不替你决定 |
| `Hash` | ⚠️ 常与 `Eq` **同写** | 中高风险，键语义须稳定 |

```rust
// Ⅰ 类：放心写
#[derive(Debug, PartialEq, Eq)]
struct Order { id: u64, symbol: String }

// Ⅲ 类：Copy 别给带 String 的订单乱加
// #[derive(Copy)]  // ❌ String 非 Copy → 编译失败，且一旦曾加上会锁死演进
```

---

## 三大类一览

| 类别 | Trait | 策略 |
|------|-------|------|
| **Ⅰ 几乎总是该有** | `Debug` · `PartialEq` / `Eq` | 公开类型默认加上 |
| **Ⅱ 多线程默认假设** | `Send` · `Sync` | 多数自动满足；不能满足须文档说明 |
| **Ⅲ 谨慎对待** | `Copy` · `Hash` | 长期约束；按需 |

---

## 场景速记（量化 / 交易域）

| 场景 | 缺什么 / 错加什么 | 后果 |
|------|-------------------|------|
| 订单 struct 无 `Debug` | Ⅰ 类缺失 | 策略调试、`assert_eq!` 失败信息无法打印 |
| 订单 struct 无 `PartialEq` | Ⅰ 类缺失 | 信号去重、回放对比写不顺 |
| 行情模块非 `Send` | Ⅱ 类不满足 | 无法 `spawn` 到策略线程、订单线程 |
| 带 `String` 的订单误 `Copy` | Ⅲ 类错加 | 类型演进锁死；改 `Arc<str>` 全链路报错 |
| 订单 ID 作 map 键但 `Hash`≠`Eq` | Ⅲ 类错配 | `HashMap` / `HashSet` 查找异常 |

---

## 一、几乎总是该实现（Ⅰ 类）

### 1. `Debug`

**作用**：支持 `{:?}` / `{:#?}` — 日志、调试、单元测试断言失败时的自动输出。**任何公开类型都应提供**，哪怕手动写简化版。

```rust
#[derive(Debug)]
struct User {
    id: u64,
    name: String,
}

fn main() {
    let u = User { id: 1, name: "Alice".into() };
    println!("{:?}", u); // User { id: 1, name: "Alice" }
    assert_eq!(u.id, 2, "用户信息：{u:?}"); // 断言失败会打印 Debug
}
```

**手动实现** — 隐藏敏感字段：

```rust
struct Secret {
    token: String,
}

impl std::fmt::Debug for Secret {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Secret")
            .field("token", &"******")
            .finish()
    }
}
```

---

### 2. `PartialEq` / `Eq`

| Trait | 作用 |
|-------|------|
| **`PartialEq`** | `==` / `!=`；允许「部分相等」（如浮点 `NaN`） |
| **`Eq`** | 标记**全序相等** — 自反、对称、传递；**无 NaN 式例外** |

```rust
#[derive(Debug, PartialEq, Eq)]
struct User {
    id: u64,
    name: String,
}

fn main() {
    let u1 = User { id: 1, name: "Alice".into() };
    let u2 = User { id: 1, name: "Alice".into() };
    let u3 = User { id: 2, name: "Bob".into() };

    assert!(u1 == u2);
    assert!(u1 != u3);
}
```

**作 `HashMap` / `HashSet` 键** — 还须 `Hash`（见第三节）：

```rust
use std::collections::HashMap;

#[derive(Debug, PartialEq, Eq, Hash)]
struct UserKey {
    id: u64,
    name: String,
}

let mut map = HashMap::new();
let key = UserKey { id: 1, name: "Alice".into() };
map.insert(key, "管理员");
```

**浮点特例**：`f64` 只能 `PartialEq`，**不能** `Eq`（`NaN != NaN`，违反自反性）。

---

## 二、多线程默认契约（Ⅱ 类）：`Send` + `Sync`

| Trait | 含义 |
|-------|------|
| **`Send`** | 所有权可安全**转移到**其他线程 |
| **`Sync`** | **`&T`** 可安全跨线程共享（`T: Sync` ⇔ `&T: Send`） |

**规则**：

- 绝大多数由字段组合**自动** `Send + Sync` — 这是 Rust 并发模型的**默认契约**，不是可选插件；
- 若**不能**实现 → rustdoc 写明原因 + 替代（如 `Rc` → `Arc`；单线程行情缓冲 → 跨线程用 channel 传 owned 数据）。

### 示例 1：默认安全

```rust
#[derive(Debug, PartialEq, Eq)]
struct User {
    id: u64,
    name: String,
}

fn test_send() {
    let u = User { id: 1, name: "test".into() };
    std::thread::spawn(move || {
        println!("{u:?}");
    })
    .join()
    .unwrap();
}
```

### 示例 2：`Rc` 非 `Send`

```rust
use std::rc::Rc;

let data = Rc::new(123);
// ❌ Rc 不实现 Send
// std::thread::spawn(move || println!("{data}"));

use std::sync::Arc;
let safe = Arc::new(123);
std::thread::spawn(move || println!("{safe}")).join().unwrap();
```

---

## 三、谨慎对待（Ⅲ 类）

### 1. `Copy` ⚠️

**约束**（**长期、难撤回**）：

- 实现 `Copy` 后，将来加 `String` / `Vec` 等非 `Copy` 字段 → **编译失败**（API / 内存模型被锁死）；
- 赋值、传参**隐式复制** — 量化里大 struct 或含指针的类型可能带来性能与语义意外；
- 典型踩坑：给带 `String` 的**订单 struct** 加了 `Copy`，后续想改成 `Arc<str>` / 引用计数 → 全链路报错。

**仅推荐**：无堆分配、稳定小值 — `i32`、`Point { x, y: f32 }`、简单 C 风格枚举。

```rust
// ✅ 适合 Copy
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct Point {
    x: f32,
    y: f32,
}

// ❌ 含 String 无法 derive Copy
// #[derive(Copy)]
// struct User { id: u64, name: String }  // 编译错误
```

**底层**：`Copy: Clone` — 复制成本极低；实现后 move 语义在赋值/传参处变为 bitwise 拷贝。

---

### 2. `Hash` ⚠️

**规则**：`Hash` 必须与 `PartialEq` / `Eq` **完全一致** — 相等 ⇒ 哈希相同；否则 `HashMap` 查找异常。

```rust
#[derive(Debug, PartialEq, Eq, Hash)]
struct User {
    id: u64,
    name: String,
}

use std::collections::HashSet;

let mut set = HashSet::new();
set.insert(User { id: 1, name: "Alice".into() });
```

**危险场景** — 键入集合后**修改参与哈希的字段**：

```rust
#[derive(Debug, PartialEq, Eq, Hash)]
struct BadKey {
    id: u64,
    name: String,
}

let mut set = HashSet::new();
let mut key = BadKey { id: 1, name: "Alice".into() };
set.insert(key.clone());
key.name = "Bob".into(); // 改了字段，但 set 里仍按旧哈希存着 → 逻辑上「丢键」
```

**对策**：哈希键类型**不暴露**可随意 mut 的字段；或键用不可变 newtype / 只读视图。

---

## 四、可直接复制的标准模板

### A. 通用公开结构体（默认首选）

```rust
/// 对外 API 的常规值类型：Debug + 相等，暂不 Copy/Hash。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Widget {
    id: u64,
    label: String,
}
```

### B. 需要作 `HashMap` / `HashSet` 键

```rust
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct WidgetId {
    id: u64,
    namespace: String,
}
```

### C. 纯栈小值（可 Copy）

```rust
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Coord {
    pub x: i32,
    pub y: i32,
}
```

### D. 含敏感字段（手动 Debug）

```rust
pub struct ApiToken {
    inner: String,
}

impl std::fmt::Debug for ApiToken {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ApiToken").field("inner", &"<redacted>").finish()
    }
}

impl Clone for ApiToken { /* … */ }
// PartialEq / Eq 按业务决定是否派生
```

### E. 非 `Send` 类型（须在文档说明）

```rust
use std::rc::Rc;

/// 单线程共享；**非 Send**。跨线程请用 `Arc<Inner>` 或克隆数据。
pub struct SharedCounter {
    inner: Rc<CounterState>,
}
```

---

## 五、开发落地清单

1. **Ⅰ 类** — 对外 struct / enum → **`#[derive(Debug, PartialEq, Eq)]`**（+ 常用 `Clone`）；
2. **Ⅱ 类** — 多线程 crate 默认假设 `Send + Sync`；行情→策略→下单链路尤其如此；例外写进 rustdoc；
3. **Ⅲ 类 `Copy`** — 仅稳定小栈值；含堆 / 订单体一律 **`Clone`** 不 **`Copy`**；
4. **Ⅲ 类 `Hash`** — 要当 map/set 键再加，与 **`Eq` 成对**；键字段入集合后不可静默修改；
5. 敏感字段 → **手动 `Debug`** 脱敏。

---

## 速记

| | 影响 | 代价 |
|---|------|------|
| Ⅰ `Debug`/`Eq` | 只增便利 | 不加难调试 |
| Ⅱ `Send`/`Sync` | 线程契约 | 非 Send 不能 spawn |
| Ⅲ `Copy`/`Hash` | 锁死/不变式 | 难演进 / map 失效 |

**derive**：Debug·Eq ✅ · Copy 显式 · Hash+Eq 成对 · 模板 A~E 见上文 §四

→ 回 [02 通用 Trait](./02-common-traits-for-types.md)
