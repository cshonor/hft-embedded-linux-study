# Item 2 · 方法与三种 self

← [Item 2 目录](./README.md) · 逻辑脉络 → [02-logic-flow.md](./02-logic-flow.md)

> **权威来源**：[The Book 5.3 方法语法](../../../00-Book/05-structs/5.3-方法语法.md)

---

## 一、方法的本质

> **方法：通过 `impl 类型` 绑定到结构体 / 枚举，依靠 `self` / `&self` / `&mut self` 关联实例。**

| 要点 | 说明 |
|------|------|
| **绑定载体** | `impl 结构体名` 或 `impl 枚举名` 块，把一组函数「挂」到该类型上 |
| **实例关联** | 方法**第一个参数**必须是 `self` / `&self` / `&mut self`（可写全 `self: &Self` 等） |
| **调用方式** | `实例.方法()` —— 实例自动作为 `self` 传入 |
| **`Self`** | `impl` 块内的类型别名，指**正在被实现的那个类型** |

和自由函数对比：

| | 自由函数 | 方法 |
|---|----------|------|
| 写法 | `fn area(r: &Rectangle)` | `impl Rectangle { fn area(&self) }` |
| 调用 | `area(&r)`，手动传实例 | `r.area()`，自动关联 |
| 绑定 | 无类型归属 | 绑定到 `Rectangle` / `Message` 等 |

---

## 二、三种 self 接收者

| 接收者 | 语义 | 调用后实例 | 典型场景 |
|--------|------|------------|----------|
| **`&self`** | 不可变借用，**只读** | 继续可用 | `len()`、`area()`、`is_empty()` |
| **`&mut self`** | 可变借用，**可改字段** | 继续可用 | `push()`、`resize()`、`increment()` |
| **`self`** | 按值接收，**转移所有权** | 原绑定**失效**（除非方法 `return self`） | `into_*`、`destroy`、消费式转换 |

等价全写：

```rust
fn read(&self) { ... }           // self: &Self
fn write(&mut self) { ... }      // self: &mut Self
fn consume(self) { ... }         // self: Self
```

**借用规则**：同一时刻，`&self` 可与多个 `&self` 共存；`&mut self` 独占；`self` 消耗后不能再借用。

---

## 三、结构体示例

```rust
#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

impl Rectangle {
    // &self：只读
    fn area(&self) -> u32 {
        self.width * self.height
    }

    // &mut self：修改
    fn resize(&mut self, w: u32, h: u32) {
        self.width = w;
        self.height = h;
    }

    // self：拿走所有权
    fn destroy(self) {
        println!("销毁 {:?}", self);
    }
}

fn main() {
    let mut r = Rectangle { width: 10, height: 20 };

    println!("面积：{}", r.area()); // &self
    r.resize(30, 40);              // &mut self
    r.destroy();                   // self，r 失效
    // println!("{:?}", r);        // ❌ 编译错误：value moved
}
```

---

## 四、枚举示例（同理）

```rust
enum Message {
    Text(String),
    Quit,
}

impl Message {
    // &self：只读匹配
    fn is_quit(&self) -> bool {
        matches!(self, Message::Quit)
    }

    // self：消耗并转换
    fn into_string(self) -> Option<String> {
        match self {
            Message::Text(s) => Some(s),
            Message::Quit => None,
        }
    }
}
```

枚举方法里 `match self` / `&self` 常配合 **穷尽变体**（Item 1 ADT）。

---

## 五、关联函数（不是方法）

没有 `self` 参数的 `impl` 块内函数叫 **关联函数**（associated function），用 `Type::func()` 调用：

```rust
impl Rectangle {
    fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
}

let r = Rectangle::new(10, 20); // 不是 r.new(...)
```

→ Book [5.3 方法语法](../../../00-Book/05-structs/5.3-方法语法.md)

---

## 六、串回 Item 2 脉络

```text
自由函数（无绑定）
    → 方法（impl + self，绑定类型与实例）
    → fn 指针（无捕获）
    → 闭包（捕获环境 + Fn*）
    → Trait（抽象 + 静/动态分发）
```

方法解决的是：**行为挂在类型上、自动拿到当前实例**；后续闭包 / Trait 在此基础上做回调与多态。

---

## 易错点

| 坑 | 说明 |
|----|------|
| `self` 调用后实例失效 | 类似 `let x = vec; vec.into_iter()` 后不能再用 `vec` |
| 方法 vs 关联函数 | 有无 `self`；`Type::new()` vs `instance.method()` |
| `&mut self` 与借用冲突 | 已有 `&` 存活时不能再拿 `&mut self` |

→ 更多 → [05-pitfalls.md](./05-pitfalls.md)

## 相关

- 逻辑脉络 → [02-logic-flow.md](./02-logic-flow.md)
- 案例 → [04-examples.md](./04-examples.md)
