# Item 34 · 重点结论

← [Item 34 目录](./README.md)

### 同侧分配、同侧释放

- C `malloc` → C `free`；Rust `Box` → Rust `drop`。**禁止混用**。

### 明确宽度整数

- 优先 **`u32` / `i64`** 等固定宽度。
- 遗留 `int` → `std::os::raw::c_int` 等；**别假设**两边 `int` 同宽。

### Safe wrapper 模式

```text
struct Wrapper { ptr: *mut c_void }  // 内部裸指针
impl Wrapper {
    pub fn new(...) -> Result<Self, Error> { ... }  // 对外 safe
}
impl Drop for Wrapper { /* C free 或 Box drop */ }
```

- 不让 `extern "C"` 调用散落全库。

### Panic 不得跨 FFI

- 未捕获 panic 展开到 C → **UB**。
- 对外 `extern "C"` 入口：`catch_unwind` + 错误码，或 **`panic = "abort"`** 策略（Item 18）。

---
