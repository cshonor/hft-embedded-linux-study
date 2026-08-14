# 2.3.3 · Newtype 模式完整详解

> 所属：**Traits and Trait Bounds · 相干性** · [← 07 hub](./07-coherence-orphan-rule.md)

← [07.2 Coverage 与 Blanket](./07-2-coverage-blanket.md) · 下一节 [08 Trait 限定](./08-trait-bounds.md)

前置 → [07.1 孤儿规则](./07-1-orphan-rule.md) · Ch03 [04 Wrapper Types](../Chapter-03-Designing-Interfaces/04-wrapper-types.md)

> ER → [Item 06 Newtype](../../01-ER/Chapter-01-Types/Item-06-newtype-pattern/README.md)

---

## 一、基础定义

**Newtype（新类型模式）**：用**单字段元组结构体**包裹已存在的底层类型，创造编译器眼中**全新、独立**的自定义类型。运行时**零开销**，内存布局与内层类型一致。

```rust
struct UserId(u32);
struct Username(String);
```

编译器视 `UserId`、`u32`、`Username` 为**三种无关类型**，不能互相混用。

### 零成本抽象

`#[repr(transparent)]` 强制保证布局与内层 **1:1**，无包装开销（FFI / niche 继承场景常用）：

```rust
#[repr(transparent)]
struct UserId(u32);
```

→ layout / niche：[03 复杂类型](./03-complex-types.md)

---

## 二、四大核心用途

### 1. 类型安全 — 杜绝语义混用（最常用）

底层类型相同、业务含义不同 → Newtype 强制编译期区分，防传参顺序错乱、ID 串用。

```rust
// ❌ 无 Newtype：传反了仍编译通过
fn get_order(user_id: u32, order_id: u32) {}
fn main() {
    get_order(5008, 1001); // user/order 传反
}

// ✅ Newtype：编译期拦截
struct UserId(u32);
struct OrderId(u32);

fn get_order(user_id: UserId, order_id: OrderId) {}

fn main() {
    let uid = UserId(1001);
    let oid = OrderId(5008);
    // get_order(oid, uid); // ❌ 类型不匹配
    get_order(uid, oid);
}
```

量化常见：`Price(f64)` · `Quantity(u64)` · `Timestamp(u64)` — 把含义编码进类型系统。

### 2. 绕过孤儿规则 — 给外部类型 impl 外部 Trait

**孤儿规则**（[07.1](./07-1-orphan-rule.md)）：禁止为**外部 crate 的类型**实现**外部 crate 的 Trait**。

```rust
// ❌ Vec、Display 均外部
// impl std::fmt::Display for Vec<i32> { ... }
```

**Newtype 解法** — 包装成**本地类型**，再 impl 外部 Trait：

```rust
struct MyVec(Vec<i32>);

impl std::fmt::Display for MyVec {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "自定义Vec：{:?}", self.0)
    }
}

fn main() {
    println!("{}", MyVec(vec![1, 2, 3]));
}
```

| 你拥有 | 可做 |
|--------|------|
| **本地 Trait** | `impl MyTrait for ForeignType` |
| **本地 Type** | `impl ForeignTrait for MyType` |
| **双外部** | **必须 NewType**（或 fork / 上游 PR） |

→ demo：[orphan-rule-demo](./orphan-rule-demo/)

### 3. 封装校验 — 控制合法构造入口

字段私有 + 带校验的构造方法，保证 Newtype 始终合法：

```rust
#[derive(Debug)]
struct UserId(u32);

impl UserId {
    pub fn new(id: u32) -> Option<Self> {
        if id > 0 && id < 999_999 {
            Some(Self(id))
        } else {
            None
        }
    }
    pub fn inner(&self) -> u32 {
        self.0
    }
}

fn main() {
    assert!(UserId::new(1001).is_some());
    assert!(UserId::new(0).is_none());
}
```

> **注意**：同一定义模块内仍可直接 `UserId(0)`；跨模块调用方须走 `new()`。工程上可把 struct 设为 `pub`、构造函数 `pub fn new`、字段不 `pub`。

量化：价格非负、仓位非零 — Newtype + 私有构造。

### 4. 专属方法 — 不污染全局

```rust
struct Price(f64);

impl Price {
    pub fn fee(&self, rate: f64) -> f64 {
        self.0 * rate
    }
}
```

---

## 三、工程实践：NewType + `Deref` / `AsRef`

双外部扩展后，常配合透明访问内层 API：

```rust
pub struct WrapperVec(Vec<u8>);

impl std::ops::Deref for WrapperVec {
    type Target = Vec<u8>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl std::fmt::Display for WrapperVec {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "WrapperVec({})", self.0.len())
    }
}
```

⚠️ **勿滥用 `Deref`** — 会弱化 Newtype 类型安全 → ER [over_deref demo](../../01-ER/Chapter-01-Types/Item-05-type-conversions/demo/src/over_deref.rs)

### 常见场景

- 外部错误 + context：wrapper + `From` / `thiserror`  
- 扩展 Iterator：本地 trait + blanket impl → [Ch13 extension traits](../Chapter-13-Rust-Ecosystem/07-extension-traits.md)

---

## 四、配套 Trait 实现

### 派生

```rust
#[derive(Debug, Clone, Copy)]
struct UserId(u32);
```

### `Deref` 便捷取值（慎用）

```rust
use std::ops::Deref;

struct UserId(u32);

impl Deref for UserId {
    type Target = u32;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}
// *uid 得到 &u32
```

---

## 五、Newtype vs 类型别名 `type`

| | `type UserId = u32` | `struct UserId(u32)` |
|---|---------------------|----------------------|
| **类型关系** | 同一类型，仅别名 | **全新独立类型** |
| **类型安全** | 无隔离，互通 | 强隔离 |
| **绕过孤儿规则** | ❌ 本质仍是 `u32` | ✅ 本地自定义类型 |

```rust
type Uid = u32;
let a: Uid = 1;
let b: u32 = a; // ✅ 互通

struct UidNew(u32);
let x = UidNew(1);
// let y: u32 = x; // ❌ 不能直接赋值
```

---

## 六、常见误区

| 误区 | 纠正 |
|------|------|
| 「孤儿规则禁止一切外部 impl」 | 只禁**双外部** |
| 「NewType 有运行时开销」 | **零成本**；`repr(transparent)` 保 ABI |
| 「`type` 别名 = Newtype」 | 别名无类型安全、不能绕孤儿 |
| 「Deref 越多越好」 | 过度 Deref 绕过校验 |

---

## 七、学习路线（量化向）

```text
1. 入门 — UserId / Price / Qty 类型隔离，防传参 bug
2. 进阶 — 绕孤儿规则 + 外部 Trait
3. 工程 — 私有构造 + 校验，业务合法类型
4. 优化 — repr(transparent) + 谨慎 Deref/AsRef
```

---

## 八、一句话速记

> Newtype = 给底层类型套一层**编译期马甲**，运行零开销 — 防写错参数 + 绕孤儿规则 impl 外部 Trait。


```bash
cd 02-RFR/Chapter-02-Types/orphan-rule-demo && cargo run
```

---

## 速记

## 定义

单字段元组 struct · 编译期新类型 · **零开销** · `repr(transparent)` 保布局

## 四大用途

1. **类型安全** — `UserId` vs `OrderId`  
2. **绕孤儿规则** — `MyVec(Vec<T>)` + `impl Display`  
3. **校验构造** — 私有 + `new()`  
4. **专属方法** — `Price::fee()`

## vs `type` 别名

| 别名 | Newtype |
|------|---------|
| 同一类型 | 新类型 |
| 无隔离 | 强隔离 |
| 不能绕孤儿 | 可以 |

## 双外部

本地 trait **或** 本地 type **或** NewType

## Deref

便捷取值 · **勿滥用**（弱化安全）

## 自测

- [ ] 为何 `type Uid = u32` 不能防传参反了？  
- [ ] 给 `Vec<i32>` 写 `Display` 为何非法，Newtype 如何合法？  
- [ ] Newtype 有运行时开销吗？

