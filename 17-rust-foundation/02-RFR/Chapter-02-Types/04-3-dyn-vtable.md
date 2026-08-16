# 1.4.3 · `dyn Trait` · 胖指针 · vtable

> 所属：**Types in Memory · DST 与宽指针** · [← 04 hub](./04-dst-wide-pointers.md)

← [04.2 宽指针](./04-2-wide-pointers.md) · 下一节 [04.4 容器 · FFI · HFT](./04-4-containers-ffi-hft.md)

---

## 一、`dyn Trait` 是什么

**动态分发的核心** + **DST** — 编译期不知具体类型大小，**不能**按值使用：

```rust
// ❌ let obj: dyn Animal = Cat;
// ✅ 必须间接持有
let obj: &dyn Animal = &cat;
let boxed: Box<dyn Animal> = Box::new(cat);
```

`&dyn Trait` / `Box<dyn Trait>` 的**句柄**是 Sized（64 位下引用 **16B**）；**指向的** `dyn Trait` 仍是 DST。

---

## 二、胖指针布局（64 位）

```text
&dyn Animal
┌─────────────────────┬─────────────────────┐
│ data ptr (8B)       │ vtable ptr (8B)     │
└─────────────────────┴─────────────────────┘
         ↓                       ↓
   Cat / Dog 实例          Cat 的 Animal vtable（静态只读）
```

与 slice 胖指针**同形状**（两字），第二字含义不同：**len vs vtable**。

---

## 三、vtable 里有什么（概念模型）

编译器为每个 **`(具体类型, Trait)`**（+ 必要 supertrait）生成**静态只读** vtable。布局是 **rustc 内部细节**，下面为**学习用简化**：

```text
vtable for (Cat, Animal):
┌──────────────────────────────────────────┐
│ drop_in_place(ptr)     // 析构此具体类型   │
│ size_of::<Cat>()       // 对象字节数       │
│ align_of::<Cat>()      // 对齐要求         │
│ Animal::make_sound     // trait 方法 fn 指针│
│ Animal::get_name       // …               │
│ (supertrait 方法，如 Debug::fmt)          │
└──────────────────────────────────────────┘
```

| 部分 | 作用 |
|------|------|
| **drop / size / align** | `Box<dyn Trait>` drop、realloc、`size_of_val` 等需要知**具体类型**布局 |
| **trait 方法指针** | 动态调用时查表跳转 |
| **supertrait** | `trait Animal: Debug` → vtable 也含 `Debug` 方法槽位 |

---

## 四、方法调用：`obj.make_sound()` 流程

以 `let obj: &dyn Animal = &cat` 为例：

1. 从胖指针取 **`data_ptr`**、**`vtable_ptr`**
2. 在 vtable 中查 **`make_sound`** 对应槽位的 **fn 指针**
3. 以 **`data_ptr` 作 `&self`** 调用该 fn
4. 实际执行 **`Cat::make_sound(&cat)`**（或 `Dog::…`）

→ 比静态分发多 **vtable 间接**；优化器通常**难 inline** → [05.1 动态分发](./05-1-static-vs-dynamic.md)

---

## 五、对象安全（为何有些 trait 不能 `dyn`）

vtable 需要**固定数量、固定签名**的 fn 指针槽位：

| 不允许 | 原因 |
|--------|------|
| 泛型方法 `fn foo<T>(...)` | 无法为所有 `T` 列槽 |
| `fn new() -> Self` | `Self` 大小在 trait object 上未知 |
| 需要 `Self: Sized` 的方法 | `dyn Trait` **非 Sized** |

```rust
trait Animal { fn make_sound(&self); } // ✅ object-safe

trait Bad {
    fn foo<T>(&self, x: T); // ❌
    fn new() -> Self;       // ❌
}
```

→ 详述 [05.3 对象安全](./05-3-object-safety.md)

关联类型 trait 作 `dyn` 时须 **指定关联类型**：`dyn Iterator<Item = u32>` → [06 泛型 Trait](./06-generic-traits.md)

---

## 六、验证大小（注意 API 区别）

```rust
use std::mem::{size_of, size_of_val};

let obj: &dyn Animal = &cat;

size_of::<&dyn Animal>(); // 16 — 胖指针本身（64 位）
size_of_val(obj);         // Cat 的字节数 — 用 vtable 里的 size 查具体类型
// size_of_val(&obj)      // 8 — 这是「引用的引用」，不是 trait object 宽指针大小
```

→ [`layout-demo`](./layout-demo/) · `Animal` / `Cat` 示例

**unsafe 深潜**：Nomicon / rustc `VirtualPtr` — 仅调试；生产勿手搓 vtable。

→ 下一节：[04.4 容器 · FFI · HFT](./04-4-containers-ffi-hft.md)
