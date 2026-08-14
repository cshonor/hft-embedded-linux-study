# Item 12 · dyn Trait 是 DST：不能裸用

← [Item 12 目录](./README.md) · 胖指针 → [Item 8 §01](../../Chapter-01-Types/Item-08-references-pointers/01-core-concepts.md)

> **核心结论**：Rust **不存在**合法写法把 `dyn Trait` **单独**当栈上类型；它是 **DST**，必须用引用 / 智能指针包裹。

---

## 一、底层根源：不定长类型（DST）

栈上类型 **尺寸必须编译期确定**。同一 trait 的不同实现者大小可以差很多：

```rust
trait Foo {}
struct A([u8; 100]);
struct B([u8; 1000]);

impl Foo for A {}
impl Foo for B {}
```

`dyn Foo` 表示「任意实现 `Foo` 的类型」→ **大小不固定** → **DST（dynamically sized type）**。

**铁律：DST 不能作为**

- 栈上裸变量
- 函数参数的裸类型（`f: dyn Foo` ❌）
- 返回值的裸类型（`-> dyn Foo` ❌）
- 结构体字段的裸类型（`field: dyn Foo` ❌）

必须用 **`&dyn Trait`** / **`&mut dyn Trait`** / **`Box<dyn Trait>`** / `Rc<dyn Trait>` 等**指针或容器**携带。

---

## 二、三种常见载体

### 1. `&dyn Trait` —— 不可变借用（日常首选）

```rust
fn take_foo(f: &dyn Foo) {}
```

| | |
|---|---|
| 本质 | **胖指针**（64 位机通常 16 字节）：数据指针 + vtable 指针 |
| 所有权 | **不转移**，只读借用 |
| 并发 | 允许多个 `&dyn Foo` 共存 |
| 场景 | 传参、临时多态视图；**开销最小** |

### 2. `&mut dyn Trait` —— 可变借用

```rust
fn take_foo_mut(f: &mut dyn Foo) {}
```

同样是胖指针；**独占**可变借用，同一时刻只能有一个 `&mut`。

### 3. `Box<dyn Trait>` —— 堆上 + 所有权

```rust
fn take_box(f: Box<dyn Foo>) {}

let b: Box<dyn Foo> = Box::new(A([0; 100]));
```

| | |
|---|---|
| 布局 | 实例在**堆**；`Box` 持有指向「堆上胖指针布局」的指针 |
| 所有权 | **可转移**（move `Box`） |
| 场景 | `Vec<Box<dyn Trait>>` 异构集合、生命周期跨函数、需要拥有多态值 |

还有 `Rc<dyn Trait>` / `Arc<dyn Trait>`：共享所有权 + 动态分发。

---

## 三、对比表

| 写法 | 分发 | 所有权 | 典型用途 |
|------|------|--------|----------|
| `T: Trait` / `impl Trait` | **静态** | 按 T | 性能、编译期已知类型 |
| `&dyn Trait` | **动态** | 借用只读 | 传参首选 |
| `&mut dyn Trait` | **动态** | 借用可变 | 要改内部状态 |
| `Box<dyn Trait>` | **动态** | 拥有 | `Vec` 异构、转移所有权 |

**不要混淆**：`impl Trait` / `T: Trait` 与 `dyn Trait` **无关**——前者静态单态化，后者运行期 vtable。

---

## 四、「看似直接写 dyn」的场景

### ❌ 裸 `dyn` 当参数 / 返回值

```rust
// fn bad(f: dyn Foo) {}        // 编译错误：dyn Foo 是 DST
// fn bad_ret() -> dyn Foo {}   // 同上

fn good(f: &dyn Foo) {}
fn good_box(f: Box<dyn Foo>) {}
```

### ✅ 合法的返回形式

```rust
static INSTANCE: A = A([0; 100]);

fn return_ref() -> &'static dyn Foo {
    &INSTANCE
}

fn return_box() -> Box<dyn Foo> {
    Box::new(A([0; 100]))
}
```

返回 `&dyn Foo` 必须带**生命周期**；不能返回指向临时值的引用。

---

## 五、编译演示

```rust
trait Demo {}
struct S;
impl Demo for S {}

fn main() {
    // let x: dyn Demo = S;           // ❌ DST 不能放栈上

    let x: &dyn Demo = &S;             // ✅ 引用包裹
    let y: Box<dyn Demo> = Box::new(S); // ✅ Box 包裹
    let _ = x;
    let _ = y;
}
```

---

## 六、选型口诀

1. **`&dyn Trait`** —— 借用、只读、不转移所有权 → **传参首选**
2. **`&mut dyn Trait`** —— 要改实例内容
3. **`Box<dyn Trait>`** —— 要所有权 / `Vec` 异构集合 / 生命周期复杂

> **铁律：`dyn Trait` 本身是 DST，永远不能裸用，必须被引用或智能指针包裹。**

## 相关

- 静/动态分发 → [06-dispatch-beginner-guide.md](./06-dispatch-beginner-guide.md)
- 对象安全 → [05-pitfalls.md](./05-pitfalls.md)
- Item 2 脉络 → [Item 2 §02](../../Chapter-01-Types/Item-02-express-common-behavior/02-logic-flow.md)
