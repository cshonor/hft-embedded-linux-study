# 2.1.2 · 单态化与内存布局

> 所属：**Traits and Trait Bounds · 编译与分发** · [← 05 hub](./05-compilation-dispatch.md)

← [05.1 静/动态分发](./05-1-static-vs-dynamic.md) · 下一节 [05.3 对象安全](./05-3-object-safety.md)

---

## 核心结论

不同泛型实参在编译后是**完全独立的类型、独立内存布局、独立机器码** — **不会共用同一份内存模板**。

---

## 一、泛型枚举会被单态化

Rust **没有** C++ 式「统一类模板实例」的运行时概念；编译器为**每一组不同的泛型实参**生成一份具体类型：

| 源码里写的 | 编译后 |
|------------|--------|
| `Option<u32>` | 类型 A：载荷 4B → 一套 tag+union 布局 |
| `Option<u64>` | 类型 B：载荷 8B → **另一套**布局 |

二者在类型系统里**互不相干** — `Option<u32>` 与 `Option<u64>` 不能互相赋值，内存尺寸、对齐、niche 规则都可能不同。

---

## 二、对应「标签 + 联合体」怎么变

枚举底层模型 → [03 复合类型 · 枚举](./03-complex-types.md)

```text
Option<u32>                    Option<u64>
┌──────┬──────────┐           ┌──────┬──────────┐
│ tag  │ union T  │           │ tag  │ union T  │
│      │  u32 4B  │           │      │  u64 8B  │
└──────┴──────────┘           └──────┴──────────┘
size_of 通常 8B                size_of 通常 16B（x86_64 实测见 layout-demo）
```

- 联合体大小 = **该次单态化里 `T` 的大小**（再 + tag / padding / niche）
- `u32` 与 `u64` 触发**两次**独立 layout 计算，**不共享**载荷区模板

---

## 三、跨文件（`a.rs` / `b.rs`）不影响规则

同一 **crate** 内，文件边界只是模块组织；编译器**遍历整个 crate**：

1. `a.rs` 用到 `Option<u32>` → 单态化，生成布局 + 代码
2. `b.rs` 用到 `Option<u64>` → 再单态化，**另一套**
3. 最终二进制里两份类型、两份 layout、相关函数各用各的格式

成千上万个文件用不同 `T`，就会生成对应数量的版本。

---

## 四、两个细节

| 情况 | 行为 |
|------|------|
| **相同 `T` 跨文件** | 全 crate **只生成一份** `Option<u32>` — 布局与代码**复用** |
| **`T` 是 DST**（`[u8]`、`str`、`dyn Trait`） | 不能直接作枚举载荷；须 `Box<T>` / `&T` 等 **Sized 包装** — 单态化逻辑不变，载荷变成指针 |

```rust
// ❌ enum Bad<T> { Some(T) }  // T = [u8] 不行
// ✅ Option<Box<[u8]>>         // 载荷是指针，Sized
```

---

## 五、极简对照

| | |
|---|---|
| `Option<u32>` vs `Option<u64>` | **两套**独立枚举 + **两套** layout |
| 跨文件不同 `T` | **不**共用内存结构 |
| 跨文件相同 `T` | **共用**一份 layout 与代码 |

→ 实测：`Option<u32>` / `Option<u64>` → [`layout-demo`](./layout-demo/)

→ 下一节：[05.3 对象安全](./05-3-object-safety.md)

---

## 速记

## 子节速记

```text
05.1  静态=单态化无vtable · dyn=vtable间接 · inline/LTO/体积控制
05.2  不同 T=不同类型+layout · 同 T 跨文件复用
05.3  object-safe：无泛型方法/无返回 Self 等
05.4  HFT 热路径静态 · enum 静态多态 · cargo bloat
```

## 对照表

| | 静态 | `dyn` |
|---|------|-------|
| 时机 | 编译期 | 运行期 vtable |
| 异构 Vec | enum 等 | `Vec<Box<dyn T>>` |
| HFT | ✅ 优先 | ⚠️ 慎用 |

## 自测

- [ ] 说明 `impl Trait` 是静态还是动态  
- [ ] 为何 `Option<u32>` 与 `Option<u64>` layout 不同  
- [ ] 举两个阻碍 `dyn Trait` 的 trait 方法形态

