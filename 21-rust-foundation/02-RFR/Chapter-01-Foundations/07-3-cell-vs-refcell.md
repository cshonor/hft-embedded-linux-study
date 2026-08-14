# 3.3.3 · Cell\<T\> 与 RefCell\<T\> · 计数器与底层原理

> 所属：**Borrowing and Lifetimes · 内部可变性** · [← 07 hub](./07-interior-mutability.md)

← [07.2 UnsafeCell](./07-2-unsafecell-and-containers.md) · 下一节 [07.4 应用场景](./07-4-use-cases.md)

> 本节**只讲 `Cell` / `RefCell` 本身**（不涉及 `Rc` 组合；多所有者场景见 [07.4](./07-4-use-cases.md)）。

---

## 零、三条核心结论

1. **`Cell` 内部没有任何计数器**；**只有 `RefCell` 才有读写借用计数器**。  
2. 二者都**没有打破**「多读 **互斥** 单写」的安全铁律：  
   - 普通 `let mut` + `&mut` → **编译期**检查；  
   - `RefCell` → **同一套规则**，改由**运行时计数器**检查（冲突 **panic**）；  
   - `Cell` → **不产生引用、只拷贝**，从根源避开读写共存，**不需要**计数器。  
3. 二者能实现内部可变性的共同根基：**`UnsafeCell<T>`** + 标准库内部的 **`unsafe` 裸指针操作**，对外暴露 **Safe** 的 `get`/`set` 或 `borrow`/`borrow_mut`。

---

## 一、共同底层：`UnsafeCell` + 库内 `unsafe`

正常情况下，通过 `&T` **不能**改背后的 `T`。内部可变容器在内部持有：

```rust
// 概念结构（简化）
struct UnsafeCell<T> {
    value: T,  // 编译器允许通过 &self 拿到 *mut T 修改（仅 unsafe 块内）
}
```

| 层 | 谁做 |
|----|------|
| **`UnsafeCell`** | opt-out「`&T` 永不修改」的优化假设 |
| **标准库 `unsafe` 块** | 用 `*mut T` 在 `get`/`set`/`borrow_mut` 里读写 `value` |
| **对外 API** | `Cell::get` / `RefCell::borrow` 等 — 调用方 **Safe** |

```text
你调用 .borrow_mut()  →  Safe 接口
        ↓
RefCell 内部 unsafe { *UnsafeCell::get_mut(...) }
        ↓
同时 borrow_flag 保证：此刻没有其它活跃 Ref/RefMut
```

→ 详述 [07.2](./07-2-unsafecell-and-containers.md) · Nomicon [五种 unsafe 能力](../../04-Rust-Nomicon/01_Safe_Unsafe/03-five-powers.md)

---

## 二、`RefCell<T>`：带借用计数器，运行时管控互斥

### 1. 内部结构（简化）

```rust
struct RefCell<T> {
    borrow: BorrowFlag,      // ← 唯一的「计数器」状态机
    value: UnsafeCell<T>,
}
```

`BorrowFlag` 在实现上常编码为 **有符号整数**（概念上）：

| 状态 | 含义 |
|------|------|
| `> 0` | 当前有 **n 个**活跃 `Ref`（只读借用） |
| `0` | 无活跃借用 |
| `-1` | 有 **1 个**活跃 `RefMut`（独占写） |

### 2. 计数器规则（与编译期借用**同一条铁律**）

**同一时刻**：要么多个只读，要么**至多一个**可变写 — **绝不能共存**。

#### `.borrow()` → `Ref<T>`

```text
1. 若当前为写状态（-1）→ panic
2. 否则读计数 +1
3. 返回 Ref<T>（守卫：Drop 时计数 -1）
```

#### `.borrow_mut()` → `RefMut<T>`

```text
1. 若读计数 ≠ 0，或已有写 → panic
2. 标记为写状态（-1）
3. 返回 RefMut<T>（Drop 时恢复 0）
```

#### 关键点：计数归零才能写

```rust
let c = RefCell::new(10);
let r = c.borrow();       // 读计数 = 1
// let w = c.borrow_mut(); // ❌ panic：读计数 ≠ 0
drop(r);                  // 读计数 = 0
let mut w = c.borrow_mut(); // ✅
*w = 20;
```

> **一句话**：`RefCell` 只是把**编译报错**换成**运行时计数器 panic**；读写互斥规则**一点没变**。

### 3. 为何能透过 `&RefCell` 改内部

外层是 `let x = RefCell::new(...)` 或 `&RefCell` — 编译器眼里**没有** `&mut T` 指向内部 `T`。  
`RefCell` 在 `borrow_mut` 内部用 `UnsafeCell::get` 拿到 `*mut T`，在 `unsafe` 里写入；**计数器**保证不会有两个 `RefMut` 或 `Ref`+`RefMut` 同时活跃 → 无数据竞争 UB。

### 4. API 与示例

| 方法 | 说明 |
|------|------|
| `.borrow()` | `Ref<T>`，多读共存 |
| `.borrow_mut()` | `RefMut<T>`，独占写 |
| `.try_borrow()` / `.try_borrow_mut()` | 冲突返回 `Err`，不 panic |

```rust
use std::cell::RefCell;

let s = RefCell::new(String::from("hello"));
let r1 = s.borrow();
let r2 = s.borrow();
println!("{} {}", r1, r2);
drop(r1);
drop(r2);
let mut w = s.borrow_mut();
w.push_str(" world");
```

### 5. 适用与代价

| 优点 | 缺点 |
|------|------|
| 任意 `T`，`borrow_mut` 原地改 | 计数器维护 + 冲突 **panic** |
| 可分段修改（`push`、`field`） | 仅单线程（非 `Sync`） |

---

## 三、`Cell<T>`：无计数器，拷贝规避冲突

### 1. 内部结构（简化）

```rust
struct Cell<T> {
    value: UnsafeCell<T>,   // 无 borrow_flag，无计数器
}
```

从头到尾**不存在**借用计数器。

### 2. 设计逻辑：不产生引用，全程拷贝

仅 **`T: Copy`**（`i32`、`bool`、`char` 等）— **无借用计数器**，永不 panic，与 `RefCell` 运行时计数完全不同。

| 操作 | 行为 |
|------|------|
| `.get()` | **拷贝**一份 `T` 给你 — 不是 `&T` |
| `.set(v)` | 用新副本**整体覆盖**盒内 |
| `.replace(v)` | 先拷贝出旧值，再原地覆盖 |

```text
读：拿走副本，盒内仍在
写：整体覆盖，没有「悬浮的 &T」指向旧数据
→ 不需要计数器 → 永不 panic
```

### 3. `.get()` / `.set()` / `.replace()` 内存怎么变？

`Cell` 本体（含 `UnsafeCell<T>`）常在**栈上**（`let c = Cell::new(10)`）；下面「Cell 内部」指 **托管 `T` 的那块内存**，不是另开一层堆。

#### `.get()` — 拷贝出新副本，内部原值不动

```rust
let c = Cell::new(10);
let a = c.get();
```

| 步骤 | 内存 |
|------|------|
| 1 | `Cell` 内仍是 `10`，**原地不动** |
| 2 | `.get()` **按位拷贝**一份 `10` 到栈上新变量 `a` |
| 3 | `a` 与 Cell 内是**两份独立**的 `T`；改 `a` 不影响 Cell |

✅ **`.get()` = 复制出新空间（新变量），Cell 内部不变。**

```rust
let mut a = c.get();  // 要改副本须 mut
a = 99;               // 只改 a，c 里仍是 10
```

#### `.set(新值)` — 原地覆盖 Cell 内部，不保留旧值副本

```rust
let c = Cell::new(10);
c.set(20);
```

| 步骤 | 内存 |
|------|------|
| 1 | 参数 `20` 是传入的 **Copy 副本** |
| 2 | 标准库在 `unsafe` 里对 `UnsafeCell` 内内存 **原地写入** `20` |
| 3 | 旧值 `10` 被**覆盖销毁**（`Copy` 类型无 drop 副作用）；**不会**另开一块存旧值 |

✅ **`.set()` = 原地覆盖 Cell 内 `T` 的内存，无旧值留存。**

#### `.replace(新值)` — 覆盖 + 返回旧值副本

```rust
let old = c.replace(20);
```

1. **先** `get` 式拷贝出旧值 `10` → 赋给 `old`  
2. **再** `set` 式原地覆盖内部为 `20`

### 4. 一眼看懂：完整流程

```rust
use std::cell::Cell;

fn main() {
    // Cell 内：10
    let cell = Cell::new(10);

    // get：拷贝 10 → x；cell 内仍是 10
    let mut x = cell.get();
    println!("x={}, cell={}", x, cell.get()); // 10, 10
    x = 99;                                    // 只改副本
    println!("x={}, cell={}", x, cell.get()); // 99, 10

    // set：cell 内原地 10 → 20
    cell.set(20);
    println!("cell={}", cell.get()); // 20
}
```

### 5. 为何这么设计就能不要计数器

1. **不返回引用** — `get` 只给副本，外部没有指向 Cell 内部的 `&T`  
2. **`set` 原地覆盖** — 覆盖时没有活跃引用指着旧位，无悬垂、无读写共存  
3. **无引用 → 无借用冲突 → 不需要 `BorrowFlag`**

### 6. 底层同样 `UnsafeCell` + 库内 `unsafe`

`set` / `replace` 在标准库 `unsafe` 块里写 `UnsafeCell` 内 `T`；`get` 读拷贝亦经 `unsafe` 读内部；对外 API 仍 **Safe**。

### 7. 一句话

- **`.get()`**：复制出新副本，Cell 内原值不变。  
- **`.set()`**：原地覆盖 Cell 内内存，旧值被覆盖掉。  
- **`.replace()`**：先拷贝旧值给你，再 `set` 新值。

### 8. 适用与代价

| 优点 | 缺点 |
|------|------|
| **零 panic**（无借用冲突） | 只能 `Copy` |
| 无计数器，极轻 | 不能 `borrow` 分段改 |
| 读写皆拷贝，大数据可能贵 | 不能存 `String`/`Vec` |

---

## 四、对照表（聚焦计数器与读写规则）

| 特性 | `Cell<T>` | `RefCell<T>` |
|------|-----------|--------------|
| **内部计数器** | ❌ 无 | ✅ `BorrowFlag` 读写状态 |
| 如何保证互斥 | 无引用，只拷贝 | 运行时计数：读活跃则禁止写 |
| 读写铁律 | 天然无引用冲突 | 与 `&`/`&mut` **相同**：多读 XOR 单写 |
| 违规后果 | 无此类违规 | **panic**（非 UB） |
| 底层 | `UnsafeCell` + 库内 `unsafe` | 同上 + 计数器 |
| 读写内存语义 | `get`=拷贝副本；`set`=原地覆盖 | `borrow`=共享读；`borrow_mut`=独占写 |
| `T` 限制 | `Copy` | 任意 |

---

## 五、答疑四条（对应常见疑惑）

1. **只有 `RefCell` 有借用计数器，`Cell` 没有。**  
2. **`RefCell` 计数器**：有活跃 `Ref` 时**禁止** `borrow_mut`；必须全部 `Ref`/`RefMut` 释放、状态归零才能写 — **没有放开**读写互斥。  
3. **内部可变性根源**：`UnsafeCell` 让库在 `unsafe` 里改数据；`Cell`/`RefCell` 用**不同上层策略**（拷贝 vs 计数器）保证 Safe API。  
4. **内部可变 ≠ 随便冲突读写**：`RefCell` 用计数器拦；`Cell` 用拷贝消灭引用 — 都守内存安全底线。

---

## 六、选型（本节不涉及 Rc）

```text
小 Copy 值，get/set 够用           →  Cell
String / Vec / 结构体，要借用修改  →  RefCell
能 let mut + &mut，无 &self 约束    →  优先外部可变，不用 cell
```

→ `&self`、多句柄等场景 [07.4](./07-4-use-cases.md)

---

## 七、误区

| 误区 | 纠正 |
|------|------|
| `Cell` 也有借用计数 | **没有** |
| `RefCell` 可以读写同时进行 | **不行**，冲突 panic |
| `RefCell` 放松了借用规则 | 规则相同，**检查时机**从编译期→运行时 |
| 内部可变 = unsafe 给用户用 | `unsafe` 在**标准库内部**；你调的是 Safe API |
| `Cell` 的 `get` 返回引用 | 返回 **Copy 副本** |

---

## 八、速记

1. **`Cell`：无计数器，`get`/`set` 拷贝，永不 panic。**  
2. **`RefCell`：有计数器，规则同 `&`/`&mut`，冲突 panic。**  
3. **底层都是 `UnsafeCell`；互斥铁律没变。**

---

## 自测

- [ ] 画出 `RefCell` 在 `borrow` + 未 `drop` 时调用 `borrow_mut` 为何 panic  
- [ ] 说明 `Cell` 为何不需要计数器也能安全  
- [ ] `UnsafeCell` 解决了什么问题？谁承担 `unsafe` 责任？  
- [ ] 对比：编译期 `&mut` 冲突 vs `RefCell` 运行时冲突，规则是否相同？

→ 下一节：[07.4 应用场景](./07-4-use-cases.md)
