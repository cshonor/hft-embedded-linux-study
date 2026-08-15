# 3.1 What Can Go Wrong?（什么会出错）

> 所属：**Great Responsibility** · [← 章索引](./README.md)

← [04 unsafe trait](./04-unsafe-traits.md) · 下一节 [06 有效性](./06-validity.md)

前置 → [01 unsafe 关键字](./01-unsafe-keyword.md) · [04 unsafe trait](./04-unsafe-traits.md)

---

**UB（未定义行为，Undefined Behavior）** 与 C/C++ 语义一致：一旦触发，程序后续行为**完全不受语言标准约束**，没有任何保证。

```text
unsafe ≠ 消除 UB
unsafe = 编译器不再自动校验；你仍须遵守 validity / 别名 / 布局 三大契约
```

---

## 一、UB 不一定立即崩溃

| 表现 | 说明 |
|------|------|
| **静默错误** | 看似正常运行，内部逻辑悄悄错乱；难复现 |
| **版本相关暴露** | 旧 rustc/LLVM 「碰巧」没事；升级后段错误或逻辑崩坏 |
| **优化畸变** | 编译器基于安全契约做激进优化；UB 破坏契约 → 输出违背源码逻辑 |

**关键认知**：崩溃只是 UB 的**一种**表现，**不是**必然结果。

---

## 二、UB 三大典型特征（深度）

### 1. 静默错误

最隐蔽：越界、别名违规后程序不立刻崩溃，但变量值随机错乱、计算偶发错误。

```rust
// 教学示例 — 勿在生产运行；用 Miri 检测
let mut x = 0i32;
let p = &mut x as *mut i32;
unsafe {
    let r1 = &*p;           // 共享视图
    let r2 = &mut *p;       // 与 r1 重叠的独占视图 — 违反别名规则 → UB
    let _ = (*r1, *r2);     // 可能「碰巧」打印，复杂程序中随机破坏数据
}
```

### 2. 版本相关暴露

LLVM 优化策略、标准库内部约束随版本调整。同一份含 UB 的代码，旧编译器正常，新编译器可能直接崩溃或结果全错。

### 3. 优化畸变

Rust/LLVM 默认：引用有效、别名规则成立、布局合法。UB 后优化器会基于**错误假设**裁剪、重排代码 — 出现与源码意图完全无关的行为。

→ IR 层假设：[04 Learn LLVM 17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md)

---

## 三、unsafe 三大安全契约（违反即可能 UB）

unsafe 边界的设计假设：**任何违反下列契约的操作都可能是 UB**。

### 1. Validity（有效性）→ [06](./06-validity.md)

指针 / 引用 / 值须满足：

| 要求 | 违规示例 |
|------|----------|
| 指向**已初始化、存活**的内存 | 悬垂指针、未 `write` 就 `assume_init` |
| **对齐**符合类型 | 未对齐地址上读写 `u64` |
| **范围**不越界 | 裸指针偏移出分配区 |

### 2. Aliasing（别名规则）

Rust 核心内存规则（与栈借用检查同一条铁律）：

| 同一时刻 | 允许 |
|----------|------|
| 任意多个 `&T` | ✅ |
| **至多一个** `&mut T` | ✅ |
| `&T` 与 `&mut T` 共存 | ❌ → UB |

unsafe 裸指针若制造重叠的 `&T` + `&mut T` 并同时使用 → UB。  
→ 借用检查在 `unsafe {}` 内**仍生效**：[01 §二](./01-unsafe-keyword.md)

### 3. Layout（内存布局）

| 禁止 | 示例 |
|------|------|
| 布局不兼容的类型强转 | 把 `&[u8]` 当 `&[u32]` 读（未保证对齐/长度） |
| 手改标准库内部布局 | 破坏 `Box`/`Rc`/`RefCell` 私有字段 |
| Union / 结构体字段乱访问 | 与定义排布不符的读写 |

→ 布局基础：[Ch02 §02 layout](../Chapter-02-Types/02-layout.md)

---

## 四、本章分项导航（Great Responsibility）

| 节 | 主题 | 与 UB 的关系 |
|----|------|----------------|
| **06** | [有效性](./06-validity.md) | 合法位模式、引用、enum 判别式 |
| **07** | [Panic](./07-panics.md) | **Panic ≠ UB** — panic 是明确定义的终止 |
| **08** | [转换](./08-casting.md) | 裸指针 / 引用 / `transmute` 边界 |
| **09** | [Drop 检查](./09-drop-check.md) | 双重释放、悬垂 drop、析构顺序 |

---

## 五、关键误区

| 误区 | 纠正 |
|------|------|
| `unsafe` 块会屏蔽 UB | **不消除** UB；只移交校验责任 |
| 没崩溃就没有 UB | UB 可**静默潜伏** |
| 只有裸指针会产生 UB | 错误 `unsafe impl Send`、破坏 `Rc` 计数、改标准库内部布局 → 同样 UB |
| Panic 就是 UB | **Panic 有定义**；UB 是「任何事都可能发生」 |
| 测过一次没事就安全 | 优化级别 / 编译器版本一变就可能暴露 |

---

## 六、工程规避

| # | 建议 |
|:-:|------|
| 1 | **缩小** `unsafe` 范围；封装为 Safe API，契约集中审计 → [10 管理边界](./10-manage-boundaries.md) |
| 2 | **Miri** 跑测试，提前抓潜伏 UB → [12 验证工作](./12-check-your-work.md) |
| 3 | `// SAFETY:` 分写 validity / 别名 / 布局三类前提 → [11 文档](./11-documentation.md) |
| 4 | 能用手写 unsafe 替代的安全封装（`RefCell::borrow_mut` 等）就优先用 |

```bash
# Miri 示例（项目已配 toolchain 时）
cargo +nightly miri test
```

---

## 七、核心总结

1. **UB** = 无保证；可静默、可随编译器升级暴露、可被优化放大。  
2. **三大契约**：validity · aliasing · layout — unsafe 边界默认全部遵守。  
3. **Panic ≠ UB**；裸指针不是唯一 UB 来源。  
4. **最小 unsafe + SAFETY 注释 + Miri** 是工程标配。

---

## 速记

## UB 三特征

1. **静默** — 不崩溃也错  
2. **版本** — 升级编译器暴露  
3. **优化畸变** — 契约破坏 → LLVM 乱优化  

## 三大契约

| 契约 | 一句话 |
|------|--------|
| **Validity** | 已初始化 · 对齐 · 不越界 · 不悬垂 |
| **Aliasing** | 多读 XOR 单写 |
| **Layout** | 类型布局不可乱转、乱改 |

## 误区

- `unsafe` **不**消除 UB  
- 没崩溃 **≠** 没 UB  
- Panic **≠** UB  

## 分项

`06` validity · `07` panic · `08` cast · `09` drop

## 工程

最小 unsafe · `// SAFETY:` · Miri

