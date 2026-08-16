# 小节笔记与 Demo 规范

---

## atomic（以第一、二章为范本）

| 文件 | 作用 |
|------|------|
| `本章学习笔记.md` | **章索引表**（书 § → 笔记 → demo） |
| `X.Y-english-slug/X.Y-english-slug.md` | **该节索引**；子笔记 `X.Y.Z-*.md` 同目录（标题用 `## X.Y …`，与书编号一致） |
| `X.Y-english-slug/code/*.rs` | Demo；由章根 `mod.rs` 用 `#[path = "..."]` 挂接 |

**唯一编号**：书 § = 文件名前缀 = 代码目录名。例如书 §1.7 → `1.7-mutex-rwlock/`；书 §2.1 → `2.1-atomic-load-store/`。

**第 1～10 章** 统一结构：`Chapter-NN-…/X.Y-english-slug/` 含索引 `X.Y-english-slug.md`、子笔记 `X.Y.Z-*.md`（如有）、demo 在 `code/`。维护脚本：`scripts/restructure-ch04-10-like-ch01.py`。

**禁止**：

- 同一节两套 slug（如同时保留 `1.7-send-sync` 与 `1.6-send-sync`）
- 旧稿内部编号（`§3.2`、`完整正文见本章学习笔记 §4`）出现在小节文件里
- 「仅索引、正文在章笔记」的占位 `.md` 与实体书 9 节正文文件并存

```
Chapter-01-Rust-Concurrency-Basics/
├── 本章学习笔记.md
├── mod.rs
├── 1.1-threads-in-rust/
│   ├── 1.1-threads-in-rust.md   ← §1.1 索引
│   ├── 1.1.1-spawn-join-basics.md
│   └── code/
│       └── 1.1-threads-in-rust-join-demo.rs
├── 1.7-mutex-rwlock/
│   ├── 1.7-mutex-rwlock.md
│   ├── 1.7.1-mutex-motivation.md
│   └── code/
│       └── 1.7-mutex-rwlock-mutex-demo.rs
└── …（§1.2～1.9 同结构：每节一文件夹，索引 + 子笔记 + code/ demo）
```

---

## async_tokio

| 文件 | 角色 |
|------|------|
| `本章学习笔记.md` | **章索引表**（§ → 精读 `.md` → demo 目录） |
| `X.Y-slug/X.Y-slug.md` | **该节完整精读** |
| `X.Y-slug/code/*-demo.rs` | **每节至少一个**可运行示例 |

维护：`05-Async-Concurrency-Network/scripts/restructure-02-03-like-ch01.py` · `fix-02-03-links.py`
