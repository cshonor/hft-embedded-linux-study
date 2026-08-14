# Item 11 · 易错细节

← [Item 11 目录](./README.md)

### 不要写 `x.drop()`；也不要在 `drop` 里手动 drop 字段

```rust
// x.drop(); // ❌ explicit use of destructor method
// drop(self.buf); // ❌ 字段由编译器后补，手动 → double free

std::mem::drop(x); // ✅ 按所有权消费 x，再 drop
```

- `Drop::drop` 是 **`&mut self`**：若允许手动调，对象还在作用域里却已被清理 → **僵尸对象**。
- **字段内存**：你的 `drop` 返回后编译器自动回收；**只写外部资源**（fd/锁/FFI）。
- **`mem::drop`** 先 **move** 再析构，生命周期正确结束。

### `Drop` 里不能报告失败

- `drop` **无返回值**，不能 `Result`。
- 可能失败的释放（如 DB 优雅断连）→ 提供 **`release() -> Result`** 让用户主动调；`Drop` 只做**防泄漏兜底**（best-effort）。

### 其它（待展开 §6）

- `Drop` 中再 **panic** → 可能 **double panic → abort**。
- **Async**：`Drop` 是**同步**的，**不能 `.await`** → 异步关闭需专门模式。

---
