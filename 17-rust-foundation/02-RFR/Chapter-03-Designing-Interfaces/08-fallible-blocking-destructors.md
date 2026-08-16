# 2.4 Fallible and Blocking Destructors（可失败与阻塞析构）

> 所属：**Flexible** · [← 章索引](./README.md)

`Drop` 的语义限制决定了 **I/O、网络、async 收尾** 不能单靠析构表达。

## `Drop` 不能做什么

| 限制 | 后果 |
|------|------|
| **不能返回 `Result`** | 错误无法向上传播 |
| **不能 `await`** | async 清理需显式 API |
| **panic 路径复杂** | 析构中 panic 可能 double panic / abort |

## 推荐模式

- 提供显式 **`close(self) -> Result<_, _>`** / **`shutdown`** / **`flush`**。
- 文档说明：**成功路径必须调用**；`Drop` 仅作「尽力而为」兜底（如 best-effort flush）。
- 阻塞 I/O：避免在 `Drop` 里长时间阻塞；必要时文档警告或使用 `ManuallyDrop` + 显式生命周期。

## 与 RAII 的平衡

- 锁守卫、简单资源 → `Drop` 仍合适 → [ER Item 11](../../01-ER/Chapter-02-Traits/Item-11-drop-raii/README.md)
- 文件 / socket / DB 连接 → 显式 close + `Drop` 兜底

Book → [15.3 Drop](../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) · [15.3.2 Socket RAII](../../00-Book/15-smart-pointers/15.3.2-Drop与网络Socket-RAII.md)
