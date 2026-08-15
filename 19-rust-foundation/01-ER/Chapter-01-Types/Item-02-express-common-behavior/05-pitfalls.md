# Item 2 · 易错细节

← [Item 2 目录](./README.md)

---

## 一、C++ 模板 vs Rust 泛型

| 维度 | C++ 模板 | Rust 泛型 |
|------|----------|-----------|
| **约束方式** | **隐式**（鸭子类型）：有同名方法往往就能实例化 | **显式 Trait bound**：签名写 `T: Trait`，契约前置 |
| **错误时机** | 晚：实例化到具体类型才报错，信息散、难定位 | 早：**签名处**不满足 trait 直接编译失败 |
| **多态** | 模板特化 + 虚函数（继承） | Trait **静态分发**（默认）+ **`dyn Trait` 动态分发** |
| **代码膨胀** | 每实例化一份，易膨胀 | 静态单态化可能膨胀；`dyn` 用 vtable 省代码 |

一句话：

- C++：**先用再说，错了再爆**（鸭子类型）
- Rust：**先说好规则，不符合不让过**（Trait 契约）

```rust
// Rust：想打印必须显式 bound
fn show<T: std::fmt::Display>(x: T) {
    println!("{x}");
}
// show(123); 若缺 bound，连签名都过不了
```

→ 裸 `T` 只能 move/drop → [03-key-takeaways.md](./03-key-takeaways.md) §4

---

## 二、Trait 对象的对象安全（Object Safety）

### 1. 什么是 Trait 对象？

- `dyn Trait` / `&dyn Trait` / `Box<dyn Trait>`：**类型擦除 + 运行期 vtable**
- **并非**所有 trait 都能 `dyn` —— 必须 **对象安全**

### 2. 典型非对象安全情况

#### ① 方法返回 `Self`（且无 `where Self: Sized` 排除）

```rust
trait Bad {
    fn new() -> Self;           // ❌ 运行期不知 Self 大小
    fn clone(&self) -> Self;    // ❌ Clone 因此不能 dyn Clone
}
```

#### ② 方法带未消去的泛型参数

```rust
trait Bad {
    fn foo<T>(&self, x: T);    // ❌ 每种 T 都要 vtable 条目，放不下
}
```

#### ③ Trait 要求 `Self: Sized`（supertrait）

```rust
trait SizedOnly: Sized {
    fn process(&self);
}
// let x: &dyn SizedOnly = ...;  // ❌ dyn 本身是 !Sized，与 Sized 矛盾
```

> **易混**：方法上写 `fn clone(&self) -> Self where Self: Sized` 是**把该方法排除在 vtable 外**，trait **其余方法仍可能**对象安全——与「返回 Self 且无 Sized 排除」不同。

#### ④ 使用 `Self` 作为非接收者参数

```rust
trait Bad {
    fn eq(&self, other: Self) -> bool; // ❌ 参数里裸 Self
}
```

### 3. 底层原因

`dyn Trait` = 类型擦除 + **固定签名**的 vtable 函数指针：

- 返回 `Self` → 大小未知，无法在栈上定布局
- 泛型方法 → 无限多实例，vtable 无法固定
- `trait Foo: Sized` → `dyn Foo` 不可能同时 Sized

### 4. 快速判断 checklist

对象安全（简化）：

- ✅ 方法只有 **`&self` / `&mut self` / `Box<Self>`** 等合法接收者
- ✅ **无**方法级泛型参数
- ✅ **不**返回 `Self`（或仅 `where Self: Sized` 且接受该方法不可通过 trait object 调用）
- ✅ 签名里**不**出现裸 `Self`（除接收者外）

常见**不能** `dyn` 的标准 trait：`Clone`、`Default`、`From`、`Into` 等（涉及 `Self` 或关联类型）。

### 5. 实战示例

```rust
// ✅ 对象安全
trait Draw {
    fn draw(&self);
}

struct Circle;
struct Square;

impl Draw for Circle { fn draw(&self) { println!("○"); } }
impl Draw for Square { fn draw(&self) { println!("□"); } }

fn main() {
    let shapes: Vec<Box<dyn Draw>> = vec![Box::new(Circle), Box::new(Square)];
    for s in shapes { s.draw(); }
}

// ❌ 非对象安全
trait NotObjectSafe {
    fn clone(&self) -> Self;
}
// let x: &dyn NotObjectSafe = ...;  // 编译错误
```

→ 深度选型 → [Item 12 §05](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/05-pitfalls.md)

---

## 三、闭包 `Fn*` 易错

| 坑 | 说明 |
|----|------|
| `F: FnOnce()` 参数 | 调用会**消耗**闭包，传完通常不能再用 |
| API 写死 `Fn` | 过窄；只调一次应写 **`FnOnce`** |
| `move \|\| ...` | 常为 `FnOnce`；可能移走外部变量 |

→ 兼容规则 → [02-logic-flow.md](./02-logic-flow.md) §二 · demo → [demo/](./demo/)

---

## 四、静/动态分发易混

| 误区 | 正解 |
|------|------|
| vtable 里有方法代码 | 只有**地址**；实体代码在代码段 |
| `impl` 了 trait 就一定有 vtable | 只有用到 **`dyn Trait`** 才生成 |
| 静态 / 动态二选一 | 同一类型可**并存** → [Item 12 §09](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/06-dispatch-beginner-guide.md) |
| 裸 `dyn Trait` | **DST 不能裸用** → `&dyn` / `Box<dyn>` → [Item 12 §07](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/07-dyn-trait-dst-carriers.md) |

---

## 五、速记

1. **泛型约束别忘写**——Rust 不搞鸭子类型，`T: Trait` 必须显式
2. **Trait 对象先查对象安全**——无返回 `Self`、无方法泛型、注意 `Sized` supertrait
3. **C++ vs Rust**——隐式晚报错 vs 显式早拦截
4. **回调写 `FnOnce`**，别写死 `Fn`；**vtable 只存地址**

## 相关

- 重点结论 → [03-key-takeaways.md](./03-key-takeaways.md)
- 可运行示例 → [demo/](./demo/)
