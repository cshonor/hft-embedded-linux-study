# 2.1.4 · 选型 · HFT · 汇编直觉

> 所属：**Traits and Trait Bounds · 编译与分发** · [← 05 hub](./05-compilation-dispatch.md)

← [05.3 对象安全](./05-3-object-safety.md) · 下一节 [06 泛型 Trait](./06-generic-traits.md)

---

## 一、选型速记（实战）

| 场景 | 推荐 | 说明 |
|------|------|------|
| 泛型参数 `T: Trait` | **静态** | 编译期单态化，性能最优 |
| 异构集合、插件、运行时换实现 | **`dyn Trait`** | `Vec<Box<dyn Handler>>` — 灵活，有开销 |
| 返回闭包 / 长迭代器链 | **`impl Trait`** | 隐藏具体类型，**仍静态** → [10](./10-existential-types.md) |
| 多种实现但热路径要快 | **enum + match** | 静态分发模拟多态（见下） |

---

## 二、HFT 场景要点

### 1. 热路径：静态分发优先

交易、行情、订单簿更新等核心 loop：

- 用 **`T: Strategy`** 泛型 monomorph，或 **具体类型**
- 避免 `&dyn Strategy` 在 inner loop 里反复 vtable 调用

### 2. 用 enum 做「静态多态」

不需要 runtime 插件时，用 **enum 包所有实现** + **`match`** — 编译器可优化为跳转表或直接 inline：

```rust
enum Handler {
    A(HandlerA),
    B(HandlerB),
}

impl Handler {
    fn on_tick(&mut self) {
        match self {
            Handler::A(h) => h.on_tick(),
            Handler::B(h) => h.on_tick(),
        }
    }
}
```

第三方如 **`enum_dispatch`** 可减样板 — 本质仍是静态分发。

### 3. `impl Trait` 作返回类型

复杂 iterator / 闭包链：**不引入 `dyn`**，又不必暴露具体类型名：

```rust
fn ticks() -> impl Iterator<Item = u64> { /* ... */ }
```

### 4. 何时才用 `dyn Trait`

- 插件系统、**运行时**加载模块
- 配置驱动的异构 handler 列表，且**不在**纳秒级 inner loop
- 接受 vtable + 宽指针成本，换表达力

### 5. 二进制体积：泛型组合过多时

策略层若对 **几十种 `T: Strategy`** 全 monomorph，release 二进制可从 MB 级涨到 **数十 MB**（视依赖与 LTO 而定）。HFT 常见做法：

- **订单簿 / 撮合**：1～3 个具体类型 + 泛型，或 **enum Strategy**；
- **序列化 / 日志 / 配置**：边界用 `dyn` 或具体类型，**不**把泛型传进 inner loop；
- 发版前跑 **`cargo bloat --release -n 20`** 看哪几个单态化符号占体积。

FFI 边界通常 **不用 `dyn`** → [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/README.md)

---

## 三、与汇编 / 优化的直觉（不背指令，记现象）

| | 静态 | `dyn` |
|---|------|-------|
| 典型 call | 直接 `call` 已知符号，或 **inline 消失** | `call [vtable+offset]` 类间接 |
| 优化器 | 常能 **devirtualize**（若类型可证唯一） | 难 inline 跨 vtable |
| 排查 | `cargo asm` / 06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 | 对比同逻辑泛型版 |

→ [04 Learn LLVM 17 · ch04 分发 IR 对照](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter04_ir_basic/notes/04_dispatch_static_vs_dyn.md) · [04_dispatch_O0.ll](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/ir_samples/optimize_compare/04_dispatch_O0.ll)

---

## 四、易错点

| 易错 | 纠正 |
|------|------|
| 「要 polymorphism 就必须 dyn」 | **enum / 泛型** 也是多态，且常更快 |
| `impl Trait` = 动态分发 | **静态** — 只是隐藏类型名 |
| 所有 trait 都能 `Box<dyn T>` | 须 **object-safe** → [05.3](./05-3-object-safety.md) |
| HFT 里到处 `dyn` 没问题 | 热路径 **优先 monomorph / enum** |
| `Option<u32>` 和 `Option<u64>` 共用 layout | **不同 T → 不同单态化** → [05.2](./05-2-monomorphization-memory.md) |
| `#[inline]` 一定 inline | **提示**；跨 crate 还需 LTO 等配合 |
| flow-directed monomorph 已默认启用 | **Pre-RFC 方向**；stable 仍 mainly MIR collector |

---

## 对照阅读

- ER → [Item 12 · 泛型 vs trait object](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md) · [06 dispatch 入门](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/06-dispatch-beginner-guide.md)
- Book → [17.2 trait 对象](../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md) · [10.2.3 impl Trait](../../00-Book/10-generics-traits-lifetimes/10.2.3-impl-Trait全解.md)

→ 下一节：[06 泛型 Trait](./06-generic-traits.md)
