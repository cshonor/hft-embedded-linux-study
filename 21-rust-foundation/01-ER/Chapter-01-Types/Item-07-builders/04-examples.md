# Item 7 · 案例与代码

← [Item 7 目录](./README.md)

### 消费型：顺畅链式 vs `if` 断点

```rust
// 链式
let bob = DetailsBuilder::new("Robert", "Builder", date)
    .middle_name("the")
    .preferred_name("Bob")
    .build();

// if 打断：每次 setter  move self，必须重绑
let mut builder = DetailsBuilder::new("Robert", "Builder", date);
if informal {
    builder = builder.preferred_name("Bob");
}
let bob = builder.build();
```

### 消费型：不能 build 两次

```rust
// vec![smithy.build(), smithy.build()] // ❌ use of moved value
```

### 借用型：临时值早夭

```rust
// ❌ new() 的临时 builder 在本语句结束就 drop，链上 &mut 悬空
// let bob = DetailsBuilder::new(...).middle_name("the").build();

let mut builder = DetailsBuilder::new("Robert", "Builder", date);
builder.middle_name("the");
let bob = builder.build();
```

---
