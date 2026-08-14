# 5. Summary（小结）

> [← 章索引](./README.md)

```text
为何异步 → async/await 状态机 → Pin → Pending/Waker → spawn
```

## 因果链

**`async` → 状态机 → 可能自引用 → `Pin` → `Pending` → `Waker` → 队列化 `poll` → `spawn` 提并发**

## 下一章

→ [第 9 章 Unsafe Code](../Chapter-09-Unsafe-Code/README.md)

## 索引

[01](./01-synchronous-interfaces.md)–[11](./11-spawn.md)
