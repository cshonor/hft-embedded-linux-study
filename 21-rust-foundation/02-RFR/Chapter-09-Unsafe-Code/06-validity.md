# 3.2 Validity（有效性）

> 所属：**Great Responsibility** · [← 章索引](./README.md)

← [05 什么会出错](./05-what-can-go-wrong.md) · 下一节 [07 Panic](./07-panics.md)

前置 → [05 §三 Validity 契约](./05-what-can-go-wrong.md) · [03 MaybeUninit](./03-calling-unsafe-functions.md)

---

**Validity** 定义：一块内存的**比特模式**是否符合该类型的**合法值**规范。违反 → **UB**。

```text
已初始化  ≠  Valid
Valid       ⇒  必然已初始化（合法比特）
```

| 类型 | 约束直觉 |
|------|----------|
| **引用** | 非 null、对齐、指向合法存活对象、无悬垂 |
| **`bool`** | 仅 `0` 或 `1`；非法位型破坏优化 |
| **enum** | 判别式 + 载荷须匹配同一变体 |

---

## 一、Validity 的本质

| 维度 | 含义 |
|------|------|
| **已初始化** | 内存里有可读比特 — **不**代表比特合法 |
| **有效（Valid）** | 比特严格匹配类型规定的合法格式 — **必然**已初始化 |

**例子**：内存字节为 `0x02` — **已初始化**，作为 **`bool` 无效**。

unsafe 代码须**同时**保证：

1. 内存**完整初始化**（无未提交的 `MaybeUninit` 槽位被当 `T` 读）  
2. 初始化后的比特符合该类型的 **Validity** 约束  

只保证 (1) 不保证 (2) → 仍是严重 UB。

→ Nomicon [05 未初始化内存](../../04-Rust-Nomicon/05_Uninit_Mem/README.md)

---

## 二、三类核心类型细则

### 1. 引用 `&T` / `&mut T`

四条约束，**缺一不可**：

| # | 约束 | 违规 |
|:-:|------|------|
| 1 | **非空** | 地址为 0 |
| 2 | **对齐** | 如 `u64` 引用须 8 字节对齐 |
| 3 | **指向合法对象** | 内存完整包裹一个 `T` 实例，不越界 |
| 4 | **无悬垂** | 对象生命周期未结束 |

```rust
// 教学示例 — 勿运行；Miri 会报错
let bad = 0x1 as *const u32;
// let r = unsafe { &*bad }; // 非空? 未对齐? 无对象 — 无效引用
```

---

### 2. `bool`

Rust 仅允许两种比特表示：

| 值 | 字节 |
|----|------|
| `false` | `0x00` |
| `true` | `0x01` |

`0x02`～`0xFF` → **无效 `bool`**。LLVM 假定 bool 只有 0/1，做分支裁剪、常量传播；非法位 → 优化逻辑错乱。

```rust
// 无效 bool — 教学示例，用 Miri 检测
let mut x = [0x02u8];
let bad_bool = unsafe { &*(x.as_mut_ptr() as *mut bool) };
if *bad_bool {
    // 优化器可能产生与源码意图无关的分支行为
}
```

---

### 3. 枚举 `enum`

结构：**判别式（discriminant）** + **变体载荷**。

规则：判别式必须对应**存在的变体**，载荷比特须匹配该变体布局。

```rust
enum E {
    A(u32),
    B(bool),
}

// 手动写出不存在的 tag / 与载荷不匹配的比特 → 无效 enum → UB
// （具体构造方式依赖布局，勿手搓；用 Miri 学规则）
```

→ 枚举布局：[Ch02 §03 复合类型](../Chapter-02-Types/03-complex-types.md)

---

## 三、Validity 与 LLVM 优化

Rust 把 Validity 规则转化为 LLVM 的**不可推翻假设**：

- IR 里出现类型 `T` 的值 → 优化器**认定**它满足 Validity  
- 基于此：死代码删除、常量折叠、分支合并  

无效值 → 假设崩塌 → [05 §一](./05-what-can-go-wrong.md) 所述静默错误 / 版本暴露 / 优化畸变。

→ [04 Learn LLVM 17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) · [ch05 IR 类型](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter05_ir_advanced_type/README.md)

---

## 四、三类无效值最简演示（Miri）

> **仅用于学习** — 用 `cargo +nightly miri run` 观察报错，勿依赖「没崩溃」。

```rust
// 1. 无效 bool
fn invalid_bool() {
    let mut x = [0x02u8];
    let b = unsafe { &*(x.as_mut_ptr() as *mut bool) };
    let _ = *b;
}

// 2. 无效 enum（概念：伪造 tag）
fn invalid_enum_concept() {
    enum E { A(u32), B(bool) }
    // 真实 UB 构造依赖 repr/布局；此处仅记：tag 与载荷不匹配即 UB
    let _ = std::mem::size_of::<E>();
}

// 3. 无效引用（未对齐 / 非对象地址）
fn invalid_ref() {
    let p = 0x1_usize as *const u32;
    let _ = unsafe { &*p };
}
```

---

## 五、高频误区

| 误区 | 纠正 |
|------|------|
| 初始化 = 有效 | `0x02` 初始化内存**不能**当 `bool` |
| 只有裸指针破坏 Validity | `transmute`、强转、手改 enum tag、错填 bool 内存同样 UB |
| 无效值一定崩溃 | 优化可长期掩盖；升级编译器 / 开 `-O` 后暴露 |
| `assume_init` 只检查「写过」 | 还须保证写的是**合法 `T` 位模式** |

---

## 六、工程校验

| # | 手段 |
|:-:|------|
| 1 | **Miri** — 运行时严格校验 Validity → [12 验证工作](./12-check-your-work.md) |
| 2 | 未初始化内存用 **`MaybeUninit<T>`**，避免手搓比特 → [03](./03-calling-unsafe-functions.md) |
| 3 | 跨类型转换优先 Safe API，少 `transmute` / 裸强转 → [08 转换](./08-casting.md) |
| 4 | 每段 unsafe 写清如何保证 Validity → [11 文档](./11-documentation.md) |

---

## 七、核心总结

1. **Validity** = 比特模式是否属于类型的合法值；违反 → UB。  
2. **初始化 ⊂ 有效**；有效必然初始化，反之不成立。  
3. **引用 / bool / enum** 是 unsafe 里最高频的无效值来源。  
4. LLVM 把 Validity 当优化假设 — 无效值危害常表现为**静默逻辑错**。

---

## 速记

## 一句

**比特模式是否符合类型的合法值** — 违反 → UB。

## 初始化 vs 有效

| | 已初始化 | Valid |
|---|:--------:|:-----:|
| 内存有比特 | ✅ | — |
| 比特合法 | — | ✅ |
| `0x02` 当 `bool` | ✅ | ❌ |

## 三类细则

| 类型 | 约束 |
|------|------|
| `&T` | 非空 · 对齐 · 合法对象 · 无悬垂 |
| `bool` | 仅 `0` / `1` |
| `enum` | tag 与载荷匹配 |

## 工程

`MaybeUninit` · 少 `transmute` · Miri · `// SAFETY:`

## 自测

- [ ] 为何 `assume_init` 要同时管「写过」和「位模式合法」？  
- [ ] 无效 `bool` 为何影响 LLVM 分支优化？  
- [ ] Panic 与 Validity 违反都是 UB 吗？

