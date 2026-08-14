# Item 17 · 案例与代码

← [Item 17 目录](./README.md)

### 编译期拦截「双 `&mut`」

```rust
// 两个 spawn 各要 &mut account → ❌
// cannot borrow `account` as mutable more than once at a time
```

借用检查在**未加 Mutex 前**就挡住 C++ 式并发漏洞。

### 锁倒置：`players` + `games` 两把锁

| 方法 | 顺序 |
|------|------|
| `add_and_join` | `players` → `games` |
| `ban_player` | `games` → `players` |

→ 并发死锁。

### 错误「修复」：拆锁域仍死锁 → 状态撕裂

- 用大括号让两方法**不同时持两把锁**，可避免交叉死锁。
- 但中间窗口：A 写入 `players` 后释放，B 完成封禁，A 再锁 `games` 把已封禁用户加进游戏 → **逻辑不一致**。
- 正解仍是 **单一 `State` + 统一锁顺序**，而非拆缝。

---
