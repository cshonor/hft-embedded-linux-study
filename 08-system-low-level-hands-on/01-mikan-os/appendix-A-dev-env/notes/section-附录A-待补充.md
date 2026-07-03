# 附录 A · 配置开发环境

> **全书统一：WSL2 + Ubuntu。** Windows 只作 Cursor 编辑器；编译、QEMU、EDK II 均在 WSL 里完成。  
> **不用** 官方 `mikanos-build` Ansible 黑盒 — 按 [SETUP.md](../../SETUP.md) 手动搭链。

## 要点

| 项 | 说明 |
|----|------|
| **Ch1** | `apt install llvm lld qemu-system-x86 ovmf` → [01-clang-minimal](../../chapter-01-hello-world/code/01-clang-minimal/) `make run` |
| **Ch2+ Loader** | 手动 clone EDK II + `CLANGPDB` |
| **Ch3+ 内核** | `x86_64-elf-gcc` 等（随章节补充） |
| **运行** | QEMU + OVMF（WSL 内） |

→ 完整步骤：[SETUP.md](../../SETUP.md)
