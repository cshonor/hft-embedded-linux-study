# Item 19 · 逻辑脉络

← [Item 19 目录](./README.md)

```text
其他语言：反射驱动架构（注册表、插件、ORM…）
         ↓
Rust：编译期单态化 + trait + 宏
         ↓
type_name：编译期泛型 → 对 &dyn Draw 只能看到 Draw，不是 Square
Any：      blanket impl 要求 T: 'static + ?Sized
         ↓
生命周期运行时擦除 → 禁止带非 'static 借用的 Any（防类型混淆）
```

### `type_name` 与 trait 对象

- 传入 `&dyn Draw` → 输出 **`&dyn Draw`**，底层 `Square` 信息已丢。

### `'static` 边界

- `impl<T: 'static + ?Sized> Any for T` — 故意限制；`&'a T` 与 `&'b T` 运行时不可分。

---
