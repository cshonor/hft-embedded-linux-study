# Item 6 · 案例与代码

← [Item 6 目录](./README.md)

### 物理单位防呆（火星探测器）

```rust
// ❌ 别名：编译器不拦混用
type PoundForceSeconds = f64;
type NewtonSeconds = f64;

// ✅ Newtype：混用即报错
pub struct PoundForceSeconds(pub f64);
pub struct NewtonSeconds(pub f64);

fn thrust(_: NewtonSeconds) {}

// thrust(PoundForceSeconds(1.0)); // mismatched types

impl From<PoundForceSeconds> for NewtonSeconds {
    fn from(p: PoundForceSeconds) -> Self {
        NewtonSeconds(p.0 * 4.448) // 示意换算
    }
}
```

### 孤儿规则：`Display for StdRng`

```rust
// impl fmt::Display for rand::rngs::StdRng { ... } // ❌ E0117

struct MyRng(rand::rngs::StdRng);

impl fmt::Display for MyRng {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "<MyRng instance>")
    }
}
```

### `#[repr(transparent)]`（示意）

```rust
#[repr(transparent)]
pub struct Wrapper(pub Inner);
// 与 Inner 同布局，适合 FFI / 透明传递
```

---
