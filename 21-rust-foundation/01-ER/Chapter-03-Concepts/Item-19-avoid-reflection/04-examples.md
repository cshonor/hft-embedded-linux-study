# Item 19 · 案例与代码

← [Item 19 目录](./README.md)

### `type_name` 盲区

```rust
let square = Square::new(1, 2, 2);
let draw: &dyn Draw = &square;
println!("{}", std::any::type_name_of_val(&draw));
// → "&dyn reflection::Draw"  不是 Square
```

### downcast 能力上限

```rust
let b: Box<dyn Any> = Box::new(42u32);
assert!(b.is::<u32>());
let n = b.downcast_ref::<u32>().unwrap();
// ❌ 无法问：「它还实现了 Display 吗？」并取出 dyn Display
```

---
