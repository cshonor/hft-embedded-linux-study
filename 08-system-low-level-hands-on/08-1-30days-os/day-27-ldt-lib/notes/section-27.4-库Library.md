## ④ 库（Library）· apilib.lib

拆分后 **成百 `.obj`** — Makefile **写不完**。

| 工具 | 作用 |
|------|------|
| **Librarian（库管理器）** | 把 API **.obj 归档** 成 **`apilib.lib`** |
| app 链接 | **`apilib.lib` + 自有 .obj** — 仍 **按需取成员**（静态库符号解析） |

**一次打包、处处复用** — C 运行时 **`libc.a`** 同源。

---
