# Item 11 · 重点结论

← [Item 11 目录](./README.md)

### 何时实现 `Drop`

类型掌管**非纯 Rust 堆**或**系统资源**时，封装成 RAII：

| 资源 | 示例 |
|------|------|
| OS | 文件描述符、socket | [15.3.2 Socket RAII](../../../00-Book/15-smart-pointers/15.3.2-Drop与网络Socket-RAII.md) |
| 锁 | 文件锁、DB 锁；**`MutexGuard`** |
| FFI | C 分配、需手动 free 的内存 |

### 缩小作用域，尽早 `Drop`

```rust
{
    let _guard = self.lock();
    // 仅此处持锁
} // guard drop → 解锁，后续耗时逻辑不阻塞别人
```

---
