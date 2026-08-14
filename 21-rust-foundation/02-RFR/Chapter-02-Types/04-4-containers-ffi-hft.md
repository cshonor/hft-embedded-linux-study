# 1.4.4 · 容器 · FFI · HFT 要点

> 所属：**Types in Memory · DST 与宽指针** · [← 04 hub](./04-dst-wide-pointers.md)

← [04.3 dyn 与 vtable](./04-3-dyn-vtable.md) · 下一节 [05 编译与分发](./05-compilation-dispatch.md)

---

## 一、与 `Vec` / 容器

| | 栈上句柄 | 堆上 payload |
|---|----------|--------------|
| **`Vec<T>`** | `{ ptr, len, cap }` — 24B（64 位） | 连续 `T` |
| **`&[T]`** | 宽指针 `{ ptr, len }` — 16B | 不拥有，只借用 |

`Vec` 的 `{ ptr, len, cap }` 可看作 **拥有型** slice + 容量；设计思路上也是 **数据指针 + 元数据**。

→ [第 1 章 03.1 栈/堆](../Chapter-01-Foundations/03-1-rust-memory-model.md)

---

## 二、HFT / FFI 实战要点

### 1. 性能：`dyn Trait` 与 vtable

| | 影响 |
|---|------|
| **胖指针** | 多 8B 元数据 + 读 vtable 指针 |
| **方法调用** | **两次内存相关访问**（vtable → fn ptr）+ **间接跳转** |
| **分支预测** | 目标地址运行时才定，热路径可能更差 |
| **静态 vs 动态** | 静态：代码膨胀、运行最快；`dyn`：一份 vtable、运行多间接 |

纳秒级路径：少 `dyn`、少额外 indirection；必要时用具体类型或函数指针表（可控 layout）→ [05 hub](./05-compilation-dispatch.md)

### 2. FFI：DST 不能原样过边界

**宽指针 / `dyn Trait` 是 Rust 抽象**，C 侧没有一一对应：

| Rust | 过 FFI 常见写法 |
|------|-----------------|
| `&[T]` / `&str` | `*const T` + `len`（或 `char*` + len 视协议） |
| `&dyn Trait` | ❌ 避免 — 改用 **具体类型** 或 **`extern "C" fn`** |
| `String` | 通常 `*const u8` + len，或 caller-owned buffer |

→ [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/README.md) · layout [02 · repr(C)](./02-layout.md)

### 3. 内存池 / 静态容器

- 自研 arena / ring buffer：常显式存 **`ptr + len`**（同宽指针语义），避免 DST 按值
- 预分配 + 固定 cap → **Sized** 数组或 `(ptr, len, cap)` 三元组，借鉴 `Vec` 句柄

---

## 三、与分发 / 存在类型

| 写法 | 分发 | 运行期开销 |
|------|------|------------|
| **`&dyn Trait` / `Box<dyn Trait>`** | **动态**（vtable） | 有间接调用 |
| **泛型 `T: Trait` / monomorphization** | **静态** | 无 vtable |
| **`impl Trait`（返回位置）** | **静态**（具体类型，隐藏名字） | 无 vtable → [10 存在类型](./10-existential-types.md) |

---

## 四、易错点

| 易错 | 纠正 |
|------|------|
| `&[T]` 和 `*const T` 一样大 | 切片引用是 **16B 宽指针** |
| `str` 可以 `let s: str` | 只有 **`&str` / `Box<str>`** 等 |
| FFI 传 `&[u8]` | C 收 **指针 + 长度** 两段 |
| HFT 热路径随便 `dyn` | 优先 **静态分发** |

---

## 对照阅读

- [05 编译与分发](./05-compilation-dispatch.md) · [10 存在类型](./10-existential-types.md)
- ER → [Item 12 · dyn Trait 与 DST](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/07-dyn-trait-dst-carriers.md)
- Book → [17.2 trait 对象](../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md)
- 实测 → [`layout-demo`](./layout-demo/)

→ 下一节：[05 编译与分发](./05-compilation-dispatch.md)

---

## 速记

## 子节速记

```text
04.1  DST 不能按值 · [T]/str/dyn Trait
04.2  瘦 8B · 宽 16B · len vs vtable
04.3  vtable 含 drop/size/方法 · 间接调用
04.4  Vec=ptr+len+cap · FFI 传 ptr+len
```

## 对照

| 宽指针 | 第二字 |
|--------|--------|
| `&[T]` / `&str` | len |
| `&dyn Trait` | vtable |

## 自测

- [ ] `size_of::<&[u32]>()` 在 64 位是多少？  
- [ ] 为何 `dyn Iterator` 须写 `Item = T`？  
- [ ] FFI 为何不能传 `&dyn Trait`？

