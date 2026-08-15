# 3.5 The Drop Check（Drop 检查 / dropck）

> 所属：**Great Responsibility** · [← 章索引](./README.md)

← [08 转换](./08-casting.md) · 下一节 [10 管理边界](./10-manage-boundaries.md)

前置 → [07 Panic 与不变式](./07-panics.md) · [06 Validity](./06-validity.md) · [Ch01 §04.4 Drop 顺序](../Chapter-01-Foundations/04-4-drop-order.md)

> Nomicon [05 Drop Check](../../04-Rust-Nomicon/03_Lifetime_Variance/05-drop-check.md) · Book [15.3 Drop](../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) · Pin [Ch08](../Chapter-08-Asynchronous-Programming/06-pin-unpin.md)

---

**Drop 检查器（dropck）**：静态校验带 **`impl Drop`** 的泛型类型在析构时**不会访问已失效的借用** — 防止析构悬垂引用 UB。

```text
自定义 Drop 可读所有字段
泛型持有短期 'a → 须保证 'a 长于 Self 存活
否则：编译报错（或 unsafe 下 #[may_dangle] 手动担保）
```

---

## 一、核心目标：Sound Generic Drop

### 1. 风险根源

`Drop::drop` 可读取 `&mut self` 的**所有字段**。若泛型结构持有 `&'a T`，编译器须保证：**被借数据在 `self` 析构完成前不被销毁**。否则 `drop()` 访问悬垂引用 → UB。

### 2. dropck 黄金规则（The Big Rule）

> 若泛型类型实现 **`Drop`**，则其泛型参数（生命周期 `'a`、类型 `T`）须**严格长于**该实例的存活期（`'a: 'self`、`T: 'self` 的直觉表述）。

| | |
|---|---|
| 满足规则 | 析构安全 |
| 不满足 | **默认编译失败** |
| 例外 | 你能证明 `drop()` **绝不**碰可能过期的数据 → `#[may_dangle]`（unsafe 妥协） |

### 3. 谁会被重点检查

带 **`impl Drop` 的泛型类型**（含生命周期/类型参数）是 dropck 主战场；非泛型且仅 `'static` 借用等简单情形约束更直观，但**任何** `Drop` + 引用的组合都受生命周期分析约束。

### 最小报错示例

```rust
struct Inspector<'a> {
    data: &'a u8,
}

impl<'a> Drop for Inspector<'a> {
    fn drop(&mut self) {
        println!("{}", self.data); // 析构访问借用
    }
}

fn bad() {
    let mut vec = vec![10u8];
    let i = Inspector { data: &vec[0] };
    drop(vec); // ❌ dropck：vec 先销毁，i 析构时 &vec[0] 已悬垂
}
```

---

## 二、dropck 扫描：哪些字段纳入「存活依赖」

编译器递归扫描字段，收集析构时**可能访问**的生命周期/类型：

| 字段种类 | dropck 行为 |
|----------|-------------|
| 拥有型 `T`、`Box<T>` | 要求 `T` 活到 `self` drop 完成 |
| `&'a T`、裸指针（视场景） | 捕获 `'a`，要求借用有效 |
| **`ManuallyDrop<T>`** | **不**自动 drop 内部 → **不**为 `T` 加存活约束 |
| **`MaybeUninit<T>`** | 同上（无自动析构胶水） |
| **`PhantomData<T>`** | 零大小；向 dropck/型变**声明**所有权/生命周期关系 |
| 空数组等 | 通常无额外约束 |

裸指针场景常配 **`PhantomData`**，避免编译器误判型变/dropck。

→ [Nomicon 06 PhantomData](../../04-Rust-Nomicon/03_Lifetime_Variance/06-phantom-data.md)

---

## 三、与 Unsafe 交界：绕过 / 修改 dropck

### 1. `ManuallyDrop<T>` — 屏蔽自动 Drop

| 项 | 说明 |
|----|------|
| 作用 | 离开作用域**不**自动 `Drop` |
| 场景 | 自定义内存管理、FFI、自引用、精确 drop 顺序 |
| 义务 | 须手动 `ManuallyDrop::drop(&mut slot)`，否则**泄漏** |
| dropck | 内部 `T` **不**加入存活依赖 |

### 2. `mem::forget` — 永久不 Drop

消费所有权、**永不**析构 — 堆资源泄漏。用于所有权移交 C、部分自引用/循环结构；**无视 RAII**，仅可控 unsafe 场景。

### 3. `#[may_dangle]`（nightly / 标准库内部）

`Drop` 实现**保证不读取**可能过期的泛型借用时，放宽黄金规则：

```rust
// 概念语法（标准库 Box/Vec 等使用）
// unsafe impl<#[may_dangle] 'a> Drop for Foo<'a> {
//     fn drop(&mut self) { /* 不碰 &'a 数据 */ }
// }
```

**unsafe 妥协** — 开发者担保；误用 → 析构 UB。

### 4. `Pin` + 自引用

自引用结构冲突两点：

1. **移动** → 内部指针指向旧地址  
2. **`impl Drop` + dropck** → 难表达「借用指向自身」的生命周期  

常见组合：

- **`Pin<Box<T>>`** — 禁止安全 API 移动  
- **`ManuallyDrop` / 裸指针** — 绕过 dropck 对内部借用的过严约束  
- **契约**：Pin 期间不移动；析构前指针有效 — **unsafe 担保**

→ [Ch08 Pin](../Chapter-08-Asynchronous-Programming/06-pin-unpin.md)

---

## 四、Drop 误用（与 §07 panic 安全互通）

| 风险 | 说明 |
|------|------|
| **破损状态析构** | 未初始化/非法字段被 `drop` → 读垃圾 / UB → [07](./07-panics.md) |
| **双重 Drop** | 手动 `drop_in_place` + 自动析构 → 同一对象 drop 两次 |
| **Drop 内 panic** | 析构中 panic → 可能 **abort**（双重 panic）；复杂清理用 `catch_unwind` 隔离 |

常见踩坑：错误使用 `ManuallyDrop`、裸指针所有权混乱、`ptr::write` 覆盖未 drop 的值。

---

## 五、Drop 销毁顺序（联动 Ch01）

| 场景 | 顺序 |
|------|------|
| struct 字段 | **声明顺序的逆序** |
| 数组 / `Vec` 元素 | 下标 **0 → len-1**（依次） |
| 自定义 `Drop` | 先运行 **`drop()`**，再按规则析构字段（细节见 Reference） |
| `Pin` | 内存须固定至 drop 完成 |

→ [04.4 Drop 顺序](../Chapter-01-Foundations/04-4-drop-order.md)

---

## 六、核心总结

1. **dropck**：带 `Drop` 的泛型类型 — 防止析构悬垂引用。  
2. **黄金规则**：泛型参数须长于 `Self`；例外 `#[may_dangle]`。  
3. **豁免**：`ManuallyDrop` / `MaybeUninit` 不向 dropck 强加内部 `T` 存活。  
4. **工具**：`ManuallyDrop`、`forget`、`#[may_dangle]`、`Pin`。  
5. **误用**：破损状态 drop、双重 drop — 严重 UB 风险。

---

## 速记

## dropck 是什么

带 `impl Drop` 的泛型类型 — 析构时不能访问已失效借用。

## 黄金规则

泛型参数（`'a`、`T`）须 **长于** `Self` 存活。

## 不向 dropck 强加内部析构

`ManuallyDrop<T>` · `MaybeUninit<T>` · `PhantomData`（声明用）

## unsafe 绕过/放宽

| 工具 | 作用 |
|------|------|
| `ManuallyDrop` | 不自动 drop；手动 `drop()` |
| `forget` | 永不 drop（泄漏） |
| `#[may_dangle]` | 担保 drop 不碰过期借用 |
| `Pin` | 禁移 + 自引用 |

## Drop 误用

破损状态析构 · 双重 drop · drop 内 panic

## 顺序

struct 字段：**逆声明** · Vec 元素：**正序** → [Ch01 04.4](../Chapter-01-Foundations/04-4-drop-order.md)

## 自测

- [ ] `Inspector<'a>` + `drop(vec)` 为何编译失败？  
- [ ] `ManuallyDrop` 为何能改变 dropck 约束？  
- [ ] `Unpin` 与 dropck 有何不同？

