# 1.4 The Standard Library（标准库拾遗）

> 所属：**What's Out There?** · [← 章索引](./README.md)

易被忽略但常用的 std 设施：

| API | 用途 |
|-----|------|
| **`Cow<'a, str>`** | 多数只读、偶尔拥有的字符串 API |
| **`write!` + `fmt::Write`** | 向 `String` 等追加格式化，少 `format!` |
| **`VecDeque`** | 双端 O(1)；队列 / 滑动窗口 |
| **`Arc::make_mut`** | 唯一引用原地变；否则 clone 内部（写时复制） |
| **`Option::as_deref`** | `Option<String>` → `Option<&str>` 视图 |

Book 查缺 → [`00-Book/Book-本书目录.md`](../../00-Book/Book-本书目录.md) · ER → [Item 10 标准 trait](../../01-ER/Chapter-02-Traits/Item-10-standard-traits/README.md)
