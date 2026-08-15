# 3.2.1 · 方法接收者：`self` / `&self` / `&mut self`

> 所属：**Borrowing and Lifetimes** · [← 章索引](./README.md)

← [06 可变引用](./06-mutable-references.md) · 下一节 [07 内部可变性](./07-interior-mutability.md)

前置 → [04 所有权](./04-ownership.md) · [05 `&T`](./05-shared-references.md) · [06 `&mut T`](./06-mutable-references.md)

---

`impl` 里的关联方法，对实例有三种接收形式 — 对应**所有权转移 / 不可变借用 / 可变借用**三种权限：

| 写法 | 本质 |
|------|------|
| `self` | 拿走所有权（消耗型） |
| `&self` | 不可变共享借用（只读，最常用） |
| `&mut self` | 可变独占借用（可读可写） |

---

## 一、`self` — 拿走所有权（消耗型方法）

参数直接写 `self`，没有 `&`、没有 `mut`。

### 规则

| 要点 | 说明 |
|------|------|
| 调用时 | 实例**所有权移入**方法 |
| 方法结束 | `self` 随栈帧销毁；**外部原变量失效** |
| 方法内 | 可随意修改、拆解、销毁结构体 |

### 典型场景

消费资源、收尾销毁、转换成别的类型 — 调用后实例**不再可用**。

```rust
struct User {
    id: u32,
}

impl User {
    fn into_id(self) -> u32 {
        self.id
    }
}

fn main() {
    let u = User { id: 100 };
    let num = u.into_id();
    // println!("{}", u.id); // ❌ u 所有权已移走
}
```

命名惯例：消耗型转换常带 `into_` 前缀（如 `into_inner`、`into_boxed_slice`）。

→ 所有权复习 [04.2 Move/Copy](./04-2-move-copy-clone.md)

---

## 二、`&self` — 不可变借用（只读方法）

参数：`&self` — 共享只读引用。

### 规则

| 要点 | 说明 |
|------|------|
| 所有权 | **不转移**；方法结束外部变量仍可用 |
| 修改 | 方法内**只能读**字段，不能直接改 |
| 并发借用 | 同一实例可同时存在**多份** `&self` |

### 典型场景

查询、读取属性，不改动数据 — **默认首选**。

```rust
impl User {
    fn get_id(&self) -> u32 {
        self.id
    }
}

fn main() {
    let u = User { id: 100 };
    let a = u.get_id();
    let b = u.get_id();
    println!("{}", u.id); // ✅ 所有权仍在
}
```

→ 共享借用 [05](./05-shared-references.md)

---

## 三、`&mut self` — 可变借用（修改方法）

参数：`&mut self` — 独占可变引用。

### 规则

| 要点 | 说明 |
|------|------|
| 所有权 | **不转移**；只临时拿到修改权 |
| 修改 | 方法内可读可写所有字段 |
| 独占 | 同一时刻**至多一份** `&mut self`；不能与任何 `&self` / `&mut self` 共存 |

### 典型场景

更新结构体内部字段。

```rust
struct User {
    id: u32,
    name: String,
}

impl User {
    fn rename(&mut self, new_name: &str) {
        self.name = new_name.to_string();
    }
}

fn main() {
    let mut u = User { id: 1, name: "tom".into() };
    u.rename("jerry");
    println!("{}", u.name); // jerry
}
```

**注意**：调用方绑定须 `let mut u` — 外部可变绑定才能借出 `&mut self`。

→ 可变借用 [06](./06-mutable-references.md)

---

## 四、三者对比

| 写法 | 所有权 | 能否改字段 | 同一时刻几份 | 典型用途 |
|------|--------|------------|--------------|----------|
| `self` | **转移**；外部变量失效 | 能改、能销毁 | 调用即消耗 | `into_*`、消费、析构收尾 |
| `&self` | 保留，仅借用 | 只读 | 多份共享 | 查询、读取 |
| `&mut self` | 保留，独占可变借用 | 可读可写 | **至多 1 份** | 更新内部状态 |

```text
self      → 04 所有权：move 进方法
&self     → 05 共享借用：多读
&mut self → 06 可变借用：单写
```

---

## 五、与内部可变性：`&self` 签名下仍能改字段

方法签名是 `&self`（整体只读借用）时，若某字段包了 `Cell` / `RefCell`，**仍可在方法内修改该字段** — 内部可变性的经典入口。

```rust
use std::cell::RefCell;

struct User {
    id: u32,
    visit_cnt: RefCell<u32>,
}

impl User {
    // 签名仍是 &self
    fn add_visit(&self) {
        *self.visit_cnt.borrow_mut() += 1;
    }
}
```

| 层面 | 看到什么 |
|------|----------|
| 方法签名 | `&self` — 结构体绑定层只读 |
| 字段层 | `RefCell` 在运行时借出 `RefMut`，改盒内计数 |

外部看方法是只读调用；内部改的是容器里的数据 — 详见 [07.4 应用场景](./07-4-use-cases.md)。

---

## 六、选型口诀

1. **只读不改** → 优先 `&self`  
2. **要改字段、签名能改** → `&mut self`（调用方 `let mut`）  
3. **调用完实例没用了** → `self`（消耗型）  
4. **签名被钉死 `&self` 却要改状态** → 字段包 `Cell` / `RefCell` → [07](./07-interior-mutability.md)

---

## 自测

- [ ] 写出 `into_id(self)` 调用后为何不能再访问 `u`  
- [ ] 说明 `rename(&mut self)` 为何要求 `let mut u`  
- [ ] `&self` 方法里 `RefCell` 字段能改、普通 `u32` 字段不能改 — 各是什么规则在管？

→ 下一节：[07 内部可变性](./07-interior-mutability.md)
