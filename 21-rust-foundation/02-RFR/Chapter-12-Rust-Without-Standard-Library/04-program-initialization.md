# 3.2 Program Initialization（程序初始化）

> 所属：**The Rust Runtime** · [← 章索引](./README.md)

裸机常无 `std` 的 **`lang_start → main`** 路径。

## 常见要素

- **`#![no_main]`** — 自定义入口
- **链接脚本** — 段布局、栈顶
- **启动汇编 / `#[export_name]`** — 复位向量
- **`.bss` / `.data` 初始化** — 由 runtime crate（如 `cortex-m-rt`）完成

细节因 **target** 与 **HAL** 而异 — 本章给路线图，非板卡手册。
