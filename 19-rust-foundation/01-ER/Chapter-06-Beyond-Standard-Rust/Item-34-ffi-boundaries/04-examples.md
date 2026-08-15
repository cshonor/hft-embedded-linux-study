# Item 34 · 案例与代码

← [Item 34 目录](./README.md)

### 字符串：`String` vs C string

| Rust | C |
|------|---|
| UTF-8 + length，可含 `\0` | `\0` 结尾 `char*` |

- 传出：**`CString::new(s)?`** → `into_raw()` 给 C。
- 借入：**`CStr::from_ptr`**（unsafe，须保证指针有效、UTF-8 或按字节处理）。

### `Box` ↔ C 所有权

```rust
// ❌ 返回 &raw from Box 局部变量 → 函数结束 drop → UAF

// Rust → C：交出所有权
let ptr = Box::into_raw(b);

// C → Rust：收回并释放
let b = unsafe { Box::from_raw(ptr) };
// drop(b) 释放堆
```

### 勿给 C 指针假生命周期

```rust
// ❌ C 临时指针 / 已 free
let s: &FfiStruct = unsafe { &*p };
```

- 裸指针**没有** `'a`；别假装有借用检查保护。

---
