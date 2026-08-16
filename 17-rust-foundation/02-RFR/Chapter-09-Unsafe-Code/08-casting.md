# 3.4 Casting（布局与转换）

> 所属：**Great Responsibility** · [← 章索引](./README.md)

← [07 Panic 与不变式](./07-panics.md) · 下一节 [09 Drop 检查](./09-drop-check.md)

前置 → [05 §三 Layout 契约](./05-what-can-go-wrong.md) · [Ch02 §02 Layout](../Chapter-02-Types/02-layout.md)

> Book [19.3 高级类型](../../00-Book/19-advanced-features/19.3-高级类型.md) · Nomicon [repr 系列](../../04-Rust-Nomicon/README.md) · Miri strict provenance

---

核心主题：**内存布局约束** + **类型/指针转换安全边界**。

```text
repr(Rust)  →  无稳定布局，禁止跨泛型transmute
repr(C)     →  FFI / 协议 / 硬件映射
指针 as     →  对齐 + provenance
transmute   →  按位重解释，须尺寸相等 + 布局可信
```

---

## 一、`#[repr(Rust)]` 默认布局

### 核心规则

| 规则 | 说明 |
|------|------|
| **无稳定保证** | 字段顺序、padding、对齐可由编译器按版本/优化调整 |
| **泛型不兼容** | `Foo<A>` 与 `Foo<B>` 即使 `size_of` 相同，也**不能**假定可安全 `transmute` |
| **适用** | 纯 Rust 内部逻辑，不跨 FFI、不做可移植字节协议 |

```rust
struct Data {
    a: u8,
    b: u64,
}
// 编译器可能重排字段、插入 padding
// transmute Data -> [u8; N] 不可移植、不稳定 → UB 风险
```

**绝对不能用于**：二进制协议、C 交互、裸内存 reinterpret（无 `repr(C)` 时）。

→ 详述 [Ch02 §02 Layout](../Chapter-02-Types/02-layout.md)

---

## 二、稳定布局：`#[repr(C)]` 等

### 适用场景

FFI C 互操作 · 二进制协议解析 · 硬件寄存器映射 · 需可预测字节的 `transmute`。

### `#[repr(C)]` 特性

| 特性 | 说明 |
|------|------|
| 字段顺序 | **源码声明顺序**不变 |
| padding/对齐 | 与平台 C ABI 一致 |
| FFI | `extern "C"` 传参布局兼容 |
| 泛型 | 加 `repr(C)` 后同尺寸泛型参数**可能**可字节转换 — 仍须 `unsafe` 审计尺寸/对齐/有效性 |

### 其它 `repr`

| `repr` | 用途 | 注意 |
|--------|------|------|
| `#[repr(u8/u16/u32)]` | 枚举判别式固定宽度 | 与变体布局配合设计 |
| `#[repr(packed)]` | 尽量无 padding | **极易未对齐访问 UB** |
| `#[repr(transparent)]` | 单字段 newtype 与内层同布局 | niche 继承 |

**规范**：需要可预测内存的类型 → **显式** `repr` + 查平台 ABI / 参考手册。

---

## 三、指针 cast（`as`）安全约束

### 1. 对齐（Alignment）

转型后：**目标类型的对齐要求 ≤ 当前指针对齐**。

| 转换 | 通常 |
|------|------|
| `*mut u64` → `*mut u8` | ✅ 可放宽对齐 |
| `*mut u8` → `*mut u64` | ❌ 若地址非 8 对齐 → **未对齐访问 UB** |

Miri 会检测未对齐解引用。

### 2. Strict Provenance（严格出处）

指针 ≠ 单纯地址；携带 **provenance**（合法访问的内存出处/范围）。

| 操作 | 效果 |
|------|------|
| `ptr as usize` | 提取地址，**丢弃**完整 provenance（exposed） |
| `usize as ptr` | **无法**无损恢复合法 provenance |
| 解引用「整数还原」的指针 | 通常 **UB**（稳定版无通用安全替代） |

Nightly：`from_exposed_addr` 等 — 仅 FFI、分配器等经完整审计的场景。

→ Miri strict provenance 规则 · [12 验证工作](./12-check-your-work.md)

### 3. `usize` ↔ 指针：不可逆结论

1. 指针 → `usize`：丢 provenance 与地址空间信息  
2. `usize` → 指针：无完整出处 → 直接解引用 **UB**  
3. 特殊底层代码才用 exposed API，须审计全生命周期  

---

## 四、`as` vs `transmute`

| | **`as`（指针）** | **`mem::transmute`** |
|---|------------------|----------------------|
| 做什么 | 改变**指针类型**，保留 provenance（在规则内） | **按位**重解释值 |
| 检查 | 对齐等；**不**保证布局语义合法 | 编译期仅 `size_of` 相等 |
| 典型风险 | 未对齐、错误 provenance | 无 `repr(C)` 下跨类型布局 UB |
| 语境 | 常在 `unsafe` 块内解引用 | 几乎总在 `unsafe` |

```rust
let p: *const u8 = &0u8;
let q = p as *const u32; // 改类型；解引用仍须 unsafe + 对齐合法

// transmute：尺寸相同才编译；布局须你保证
// let x: Foo<A> = unsafe { mem::transmute(foo_b) }; // 无 repr(C) 时危险
```

---

## 五、安全转换决策流程

```text
仅 Rust 内部、不序列化/FFI？
  → 默认 repr(Rust)；禁止跨泛型 transmute

需要固定字节布局（协议/C/硬件）？
  → #[repr(C)] + 查平台对齐；必要时 packed（极谨慎）

指针互转？
  → 先校验对齐；避免 ptr↔usize 往返

类型字节重解释？
  → repr(C) + unsafe transmute；禁止默认布局下乱转
```

---

## 六、高频误区

| 误区 | 纠正 |
|------|------|
| `Foo<A>`/`Foo<B>` 同尺寸就能 `transmute` | **repr(Rust)** 下布局无保证 |
| `repr(C)` = 随便 transmute | 仍须 Validity、对齐、别名 |
| 指针 = 地址整数 | **Provenance**；`usize` 往返不安全 |
| `as` 转指针一定安全 | 解引用仍 `unsafe`；须对齐 |
| `packed` 省空间无代价 | 未对齐字段访问 → UB |

---

## 七、核心总结

1. **`repr(Rust)`**：无布局保证；泛型 `Foo<A>`/`Foo<B>` 不能transmute。  
2. **稳定布局**：`repr(C)` — FFI / 二进制协议。  
3. **指针 cast**：对齐 + provenance 两道门槛。  
4. **`as` 改指针类型，`transmute` 按位重解释** — 多在 unsafe 场景。

---

## 速记

## repr

| | `repr(Rust)` | `repr(C)` |
|---|--------------|-----------|
| 字段顺序 | 不保证 | 声明顺序 |
| transmute | 跨泛型 ❌ | 协议/FFI 可用（仍 unsafe） |
| 用途 | 日常 Rust | C / 二进制 / 硬件 |

## 指针 cast 两道门槛

1. **对齐** — `*u8` → `*u64` 须地址对齐  
2. **Provenance** — `ptr as usize` 丢出处；`usize as ptr` 解引用通常 UB  

## `as` vs `transmute`

| | `as` | `transmute` |
|---|------|-------------|
| 对象 | 指针类型 | 任意同尺寸位模式 |
| 保留 | provenance（规则内） | 无布局语义 |

## 决策

Rust 内部 → 默认 repr，不跨泛型 transmute  
协议/FFI → `repr(C)`  
指针 → 对齐 + 忌 usize 往返

## 自测

- [ ] 为何 `Foo<A>` 与 `Foo<B>` 不能transmute？  
- [ ] `repr(packed)` 主要风险是什么？  
- [ ] provenance 与「指针只是地址」差在哪？

