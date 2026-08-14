# Item 5 · 案例与代码

← [Item 5 目录](./README.md)

### `as`（有损）vs `Into`（trait 安全）

```rust
let x: u32 = 65_536;

let y = x as u16;        // ✅ 编译通过，高位截断（危险）
let y: u16 = x.into();   // ❌ u16 未 impl From<u32>，编译期拦截
```

### 泛型 `Into` 让调用端更顺

```rust
struct IanaAllocated(pub u64);

impl From<u64> for IanaAllocated {
    fn from(v: u64) -> Self { IanaAllocated(v) }
}

// fn is_iana_reserved(s: IanaAllocated)  // 不能直接传 42

fn is_iana_reserved<T: Into<IanaAllocated>>(s: T) {
    let s = s.into();
    // ...
}

is_iana_reserved(42);              // ✅
is_iana_reserved(IanaAllocated(0)); // ✅ 自反 From
```

### `TryFrom`（示意）

```rust
// u16::try_from(large_u32) -> Result<u16, TryFromIntError>
```

---
