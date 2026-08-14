# 2.1.1 · 静态分发 vs 动态分发

> 所属：**Traits and Trait Bounds · 编译与分发** · [← 05 hub](./05-compilation-dispatch.md)

← [04 DST 与宽指针](./04-dst-wide-pointers.md) · 下一节 [05.2 单态化与内存](./05-2-monomorphization-memory.md)

---

## 一、两种分发模型

### 1. 静态分发（Monomorphization / 单态化）

**原理**：编译器为每个具体类型 `T` **生成一份专属代码**。

```rust
fn process<T: Handler>(h: T) {
    h.on_tick();
}
// 对 HandlerA、HandlerB 各生成一份 process 特化
```

| 优点 | 缺点 |
|------|------|
| **无 vtable 间接调用**；易 **inline** | **编译时间** ↑、**二进制**可能膨胀（代码重复） |
| 峰值延迟低 — **HFT 热路径首选** | 无法直接做「多种具体类型混在同一集合」 |

**典型写法**：`fn foo<T: Trait>(x: T)` · `impl Trait for Concrete` · 泛型 struct 字段

---

### 2. 动态分发（`dyn Trait`）

**原理**：运行时经 **vtable** 查方法地址 — 一次**间接调用**。

```rust
fn process(h: &dyn Handler) {
    h.on_tick(); // 经 vtable 跳转
}

let handlers: Vec<Box<dyn Handler>> = vec![/* 异构实现 */];
```

| 优点 | 缺点 |
|------|------|
| **一份** trait object 代码；二进制相对省 | **间接调用**；难 inline；分支预测更差 |
| **异构集合**、运行时选实现 | 依赖 **DST + 宽指针** → [04](./04-dst-wide-pointers.md) |

**典型写法**：`&dyn Trait` · `Box<dyn Trait>` · `Arc<dyn Trait>`

---

### 对照表（必背）

| | **静态** | **动态 `dyn`** |
|---|----------|----------------|
| 解析时机 | **编译期** | **运行期**（vtable） |
| 调用 | 直接 / 可 inline | 间接跳转 |
| 异构 `Vec` | 需 **enum** 等 | `Vec<Box<dyn T>>` 自然 |
| 代码体积 | 多份 monomorph | 少份 + vtable |
| HFT 热路径 | ✅ **优先** | ⚠️ 慎用 |

---

## 二、单态化深度：运行时、体积、`#[inline]`、编译器

**运行时零开销**：同一泛型函数用 `i32` 和 `f64` 各调一次 → 编译器生成 **两个独立机器码版本**；调用处类型在编译期已知，**没有**运行时类型判断、**没有** vtable 间接跳转（与 `dyn` 对比 → 上表）。

```rust
fn add_one<T: Copy + std::ops::Add<Output = T> + From<u8>>(x: T) -> T {
    x + 1.into()
}
// add_one(1i32) 与 add_one(1.0f64) → 两份特化；各自直接 call / 可 inline
```

| 维度 | 单态化 |
|------|--------|
| 调用路径 | 编译期绑定具体符号 |
| 与 `dyn Trait` | 无 vtable；可跨 crate **devirtualize + inline** |
| 代价 | **编译时间** ↑、**二进制**可能重复膨胀 |

### `#[inline]` 与 call 开销

`#[inline]` / `#[inline(always)]` 是**建议** LLVM 把函数体**嵌入调用处**，减少 `call` + 栈帧；热路径小函数常用。是否真 inline 仍由优化器决定 — 与单态化配合时，**具体类型**常让优化更容易成功。

跨 crate 默认不 inline 除非 `#[inline]` 或 LTO；HFT 可配合 **`lto = "thin"` / `"fat"`** 进一步合并。

### 控制体积：类型参数组合爆炸

泛型参数或 trait bound 组合过多时，**每个 `(T, U, …)` 组合**都可能复制一份代码 — 二进制明显变大。常见缓解：

| 手段 | 思路 |
|------|------|
| **热路径 monomorph，边界 `dyn`** | 核心 loop 用具体 `T`；插件/IO 层用 trait object |
| **enum 静态多态** | 变体有限时用 `match` 代替无限 `T` → [05.4](./05-4-selection-hft.md) |
| **少泛型层嵌套** | `Iterator` 链、`async` 状态机叠加会乘性膨胀 |
| **查体积** | `cargo bloat --release`、`-C link-arg=-Wl,--print-memory-usage` |
| **LTO / strip** | 链接期删未用单态化实例（仍不如从源头少实例） |

### 编译器如何收集单态化实例（现状 + 演进）

**当前 stable 流程**（[rustc dev guide · monomorphization](https://rustc-dev-guide.rust-lang.org/backend/monomorph.html)）：

1. **Collector** 从 crate 根（`main`、导出项等）出发；
2. **遍历 MIR**，看到 `T = Concrete` 的调用就加入待 codegen 列表；
3. **递归展开**直到闭包 — demand-driven，「用到了才生成」。

**Flow-directed monomorphization**（[Pre-RFC · Rust Internals](https://internals.rust-lang.org/t/pre-rfc-flow-directed-monomorphization-collection-in-mir/24366)）— **编译器演进方向**，尚未替代上述 collector 成为默认路径：

- 在 **MIR 层**建 **类型流图**，收集约束 `τ ⊑ α`；
- **有限解** → 可安全单态化的实例集合；
- **循环类型流**（需无限特化，如 `T → Option<T> → T…`）→ **编译期结构性报错**，而不是递归爆栈或默默膨胀。

与 `impl Trait` / `async fn` / 高阶类型流相关的「会不会无限 monomorph」问题，正是这类分析要解决的。

→ 下一节：[05.2 单态化与内存](./05-2-monomorphization-memory.md)
