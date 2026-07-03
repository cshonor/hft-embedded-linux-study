## 2.5 CLANGPDB 工具链 · 自检

> **§2 子笔记 5/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

### 全程 LLVM：EDK II 与 CLANGPDB

若 Ch1 已走 **Clang + lld-link**（WSL），Ch2 可在 EDK II 里 **继续统一 LLVM**，不必切 GCC 交叉链。

| 配置 | 作用 |
|------|------|
| **`TOOL_CHAIN_TAG = CLANGPDB`** | MikanLoaderPkg / 平台 DSC 里指定 **Clang + LLD** 工具链 |
| **`build -t CLANGPDB`** | 命令行等价 — 构建产物目录常含 `CLANGPDB` 字样 |
| **前提** | 已 `source edksetup.sh`；PATH 有 **clang**、**lld-link** |

**与 Ch1 关系：**

```
Ch1  hello.c  +  clang/lld-link  →  BOOTX64.EFI（裸 C 最小模板）
Ch2  MikanLoader  +  EDK II + CLANGPDB  →  Loader.efi（<Uefi.h> 工程化）
```

→ Ch1 工具链：[§2 二进制与 BOOTX64](../../chapter-01-hello-world/notes/section-2-二进制编辑器与BOOTX64.md) · [SETUP.md](../../SETUP.md)  
→ appendix-C 模板：[CLANGPDB 配置](../../appendix-C-edk2-files/notes/section-附录C-待补充.md)

---

### §2 自检（概念）

1. **EDK II 和 UEFI 规范什么关系？** — 规范是「接口文档」；EDK II 是 **参考实现 + 工具链 + 库**（[2.1](./section-2-1-EDK-II是什么与行业定位.md)）。
2. **MikanOS 用 EDK II 干什么？** — 编 **MikanLoader.efi** 等 UEFI 应用，不是要你重写整片主板 BIOS（[2.3](./section-2-3-MikanLoader是什么.md)）。
3. **OVMF 和 EDK II 关系？** — OVMF 是 **基于 EDK II 构建的 QEMU 用 UEFI 固件**。
4. **GetMemoryMap 在哪个服务期？** — **Boot Services**（[2.4](./section-2-4-Boot与Runtime服务.md)）。
5. **`.inf` / `.dsc` 在哪查？** — [附录 C](../../appendix-C-edk2-files/)。

---

← [2.4 Boot vs Runtime 服务](./section-2-4-Boot与Runtime服务.md) · [§2 索引](./section-2-EDK-II与MikanLoader.md) · 下一节 [3. 内存映射 · 3.1](./section-3-1-内存映射指什么与RAM视图.md)
