# 3.3.4 · 内部可变性应用场景

> 所属：**Borrowing and Lifetimes · 内部可变性** · [← 07 hub](./07-interior-mutability.md)

← [07.3 Cell/RefCell](./07-3-cell-vs-refcell.md) · 下一节 [07.5 对比与误区](./07-5-comparison-pitfalls.md)

> 本节先对比**普通借用**与 **`RefCell`**（**不涉及 `Rc`**；多所有者见文末「延伸阅读」）。`Rc` 只是多所有权工具，与读写规则无关。

---

## 零、核心矛盾（一句话）

**普通引用（编译期检查）**：只要有活跃共享引用，**编译阶段直接不让写**修改逻辑。  
**`RefCell`（运行时检查）**：允许你拿着共享引用去「尝试」修改 — 代码能编译跑起来，冲突只在**运行时**报错。

底层安全规则（多读互斥单写）**没变**；变的是**约束时机、写法、能实现的业务模式**。这就是内部可变性的真正用途。

---

## 一、普通借用 vs `RefCell`：规则一致，门槛天差地别

### 1. 普通外部借用（`let` / `&` / `&mut`）— 编译期锁死

```rust
let mut data = 10;
let r1 = &data; // 共享只读引用
data = 20;      // ❌ 编译报错，代码跑不起来
```

| 要点 | 说明 |
|------|------|
| 规则 | 有活跃 `&T` → **禁止**修改 |
| 检查时机 | **编译期**全局扫描借用生命周期 |
| 痛点 | 哪怕你能保证运行时 `r1` 马上销毁、根本不会并发读写，编译器也不认 — 语法上生命周期重叠就报错 |

### 2. `RefCell` 内部可变性 — 编译放行，运行时校验

```rust
use std::cell::RefCell;

let data = RefCell::new(10);
let r1 = data.borrow(); // 活跃读借用，Ref 守卫

// 修改代码可以正常编译
let res = data.try_borrow_mut();
// 运行时：有活跃读 → Err；无冲突则 Ok
match res {
    Ok(mut w) => *w = 20,
    Err(_) => { /* 读仍活跃，安全拒绝 */ }
}
drop(r1); // 读计数归零
*data.borrow_mut() = 20; // ✅ 无冲突，正常运行
```

| 要点 | 说明 |
|------|------|
| 规则 | **一模一样**：读活跃不能写、写活跃不能读 |
| 检查时机 | **运行时** `BorrowFlag` 计数 |
| 关键区别 | 修改逻辑**能写进源码并编译通过**；真冲突才 `panic` / `Err` |

```text
普通借用：语法上可能重叠 → 编译器宁可错杀，直接禁止写
RefCell：  编译器不扫生命周期 → 运行时只看计数器，只杀真冲突
```

→ 计数器原理 [07.3 §二](./07-3-cell-vs-refcell.md)

---

## 二、`RefCell` 不可替代的三大用途（普通借用做不到）

### 用途 1 · 签名固定 `&self`，却要改内部状态

trait、框架回调、标准库接口常把方法签名钉死在 **`fn func(&self)`** — **不能改成 `&mut self`**。

| | 普通借用 | `RefCell` |
|---|----------|-----------|
| `&self` 含义 | 整个结构体冻结，**一个字段都改不了** | 外层 `&self` 共享；字段包 `RefCell`，运行时改 |
| 能否实现 | ❌ | ✅ |

```rust
use std::cell::RefCell;

struct Logger {
    print_count: RefCell<u32>,
}

impl Logger {
    // 签名必须是 &self，不能加 mut
    fn print(&self, msg: &str) {
        println!("{}", msg);
        // 普通借用：&self 下绝对改不了成员
        *self.print_count.borrow_mut() += 1;
    }
}
```

**为何普通借用做不到**：借用检查绑在方法签名上 — `&self` 静态锁死全部修改权。`RefCell` 把修改权下沉到运行时，绕开签名带来的静态约束。→ 三种接收者形式 [06.1](./06-1-method-self-receivers.md)

小整数计数可用 `Cell<u32>` + `set(get() + 1)`，不必上 `RefCell`。

---

### 用途 2 · 运行时动态决定借用生命周期

普通借用的引用何时创建、何时销毁，必须在**编译期**被完整分析。若逻辑由运行时动态决定（缓存、事件回调、分支），编译器分析不清 — **哪怕你清楚逻辑安全**，也会报生命周期冲突。

```rust
use std::cell::RefCell;

let cache = RefCell::new(50);

if some_runtime_condition() {
    let r = cache.borrow();
    println!("{}", *r);
} // r 销毁，读计数归零

// 你自己知道：上面读已结束，此处读写不会并发
*cache.borrow_mut() = 100; // ✅ 运行时计数器同意
```

| | 普通借用 | `RefCell` |
|---|----------|-----------|
| 动态分支 + 不确定活跃引用 | 编译器分析不清 → **报错** | 编译放行；运行时看计数器 |
| 引用已销毁后的写 | 编译器未必能证明 | 计数归零 → 允许写 |

---

### 用途 3 · 细粒度可变：整体不可变，只开放个别字段

普通外部可变是**粗粒度**：一个字段要改 → 整个结构体 `let mut`，所有字段都能动，容易误改核心数据。

`RefCell` 实现**字段级**内部可变：

```rust
use std::cell::RefCell;

struct User {
    id: u64,                      // 核心：永远不可改
    name: String,                 // 核心：永远不可改
    login_times: RefCell<u32>,    // 仅统计字段可内部修改
}

let user = User {
    id: 1,
    name: "张三".into(),
    login_times: RefCell::new(0),
};

// user.id = 2;              // ❌ 编译报错
*user.login_times.borrow_mut() += 1; // ✅ 只改统计字段
```

`let mut user` 会把 `id`、`name` 也放开 — 管控粗糙；`RefCell` 精准只开放辅助状态。

---

## 三、规则一样，凭啥不一样？

| 维度 | 普通借用 | `RefCell` |
|------|----------|-----------|
| **安全底线** | 多读 XOR 单写 | **相同** |
| **约束时机** | 编译期；语法上可能重叠 → 禁止写 | 运行时；真冲突才报错 |
| **能力边界** | 受签名 + 静态生命周期分析限制 | 突破静态约束，实现上面三类架构 |
| **代价** | 零运行时开销 | 计数维护；冲突 **panic** |

**`RefCell` 的真正用处**：不是打破读写互斥，而是打破**编译期静态借用检查带来的语法限制**。

**选型原则**：能用 `let mut` + 普通借用解决的，**优先普通借用**；只有静态规则阻碍你写合理、安全的代码时，才上 `RefCell`。

---

## 四、补充场景（仍不涉及 `Rc`）

### 多句柄共享同一 `RefCell`

```rust
let data = RefCell::new(5);
let r1 = &data;
let r2 = &data;

{
    let _a = r1.borrow_mut();
    // let _b = r2.borrow_mut(); // ❌ panic
}
let _c = r2.borrow(); // ✅ mut 借用已结束
```

编译期：多个 `&RefCell` 合法；运行时：`borrow` / `borrow_mut` 仍互斥。

### `RefCell` + 非 `Copy` 原地修改

```rust
let cell = RefCell::new(vec![1, 2, 3]);
let mut w = cell.borrow_mut();
w.push(4);
```

`Cell` 做不到 `push` — 只能整体 `set` 新 `Vec`。

---

## 五、选型速查

| 需求 | 选型 |
|------|------|
| 默认、无 `&self` 约束 | `let mut` + `&mut` |
| `&self` + 小 `Copy` 计数 | `Cell<u32>` |
| `&self` + `String` / `Vec` | `RefCell<T>` |
| 动态分支、细粒度字段 | `RefCell` 包目标字段 |
| 多线程 | `Arc<Mutex<T>>` 等 — 见 [07.2](./07-2-unsafecell-and-containers.md) |

---

## 六、延伸阅读 · `Rc<RefCell<T>>`

多所有者场景：`Rc` 只给 `&T`，无法静态拿 `&mut`；叠 `RefCell` 在运行时提供内部可变 — 单线程图/树经典组合。详见 [Book 15.5](../../00-Book/15-smart-pointers/15.5-RefCell与内部可变性.md)。

```rust
use std::rc::Rc;
use std::cell::RefCell;

let data = Rc::new(RefCell::new(100));
let p1 = Rc::clone(&data);
*p1.borrow_mut() = 200;
```

---

## 四句话总结

1. 底层规矩：有活跃读就不能写、有活跃写就不能读 — **两者一模一样**。  
2. 普通引用**编译期卡死修改入口**；`RefCell` **允许写出修改代码，运行时再校验**。  
3. 用处是突破静态借用限制：`&self` 改状态、动态生命周期、细粒度字段。  
4. 能 `let mut` 就 `let mut`；静态规则挡路时才上 `RefCell`。

→ 下一节：[07.5 对比与误区](./07-5-comparison-pitfalls.md)
