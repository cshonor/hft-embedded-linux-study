# 2 · 构建无标准库可执行程序

← [本章目录](./README.md) · 上一节：[01-libc.md](./01-libc.md) · 下一节：[03-panic-handler.md](./03-panic-handler.md)

---

| 要点 | 说明 |
|------|------|
| **Nightly** | 许多平台须手动提供 **lang items**（如 `eh_personality` 栈展开） |
| **`#![no_main]`** | 禁止编译器生成默认 `main` |
| **入口符号** | 手动 `#[no_mangle] extern "C" fn _start()` / `main` / `WinMain` |
| **`compiler_builtins`** | 缺 `__aeabi_memcpy` 等链接符号时手动链接 |

→ 模板：[templates/no_main_linux.md](./templates/no_main_linux.md)（参考，需 nightly + 目标平台）
