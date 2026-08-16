# Item 15 · 案例与代码

← [Item 15 目录](./README.md)

### `mem::replace` / `Option::replace`

```rust
// item: &mut Option<Item>
// let prev = *item; // ❌ cannot move out of *item

let prev = std::mem::replace(item, Some(new_val));
// 或
let prev = item.replace(new_val);
```

在**仍持有 `&mut`** 的前提下，原子地「取出旧值、写入新值」。

### 临时 `String` 逃逸

```rust
// let found = find(&format!("..."), "ex"); // ❌ format! 临时在 ; 处 drop

let query = format!("...");
let found = find(&query, "ex");
```

---
