# 附录 C · EDK II 文件说明

> **定位：** EDK II **工程元文件** 与目录结构 — 配合 [Ch2 §2 EDK II](../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md) 阅读。

---

## 1. 三种核心元文件

| 文件 | 全称 / 角色 | 类比 |
|------|-------------|------|
| **`.dec`** | **Declaration** — Package **声明** | 库的「头文件清单 + GUID 导出」 |
| **`.inf`** | **Module Information** — **最小编译单元** | 单个 `.efi` / 驱动模块的「Makefile + 源文件列表」 |
| **`.dsc`** | **Description** — **平台/产品描述** | 整个固件或 Loader **工程总览**：选哪些 Package、用哪条工具链 |

**构建关系（简化）：**

```
.dsc（平台选模块 + 工具链）
  → 引用多个 .inf（每个 Module 编译成 .efi / .lib）
  → .dec 提供 Package 级头文件与 GUID 定义
```

---

## 2. 常见 Package（先认名字）

| Package | 作用 |
|---------|------|
| **MdePkg** | **最基础** — `<Uefi.h>`、Boot/Runtime Services、核心 Protocol |
| **MdeModulePkg** | 通用 UEFI 模块参考实现 |
| **MikanLoaderPkg** | MikanOS 书中 **Loader 源码包**（链入 edk2 树） |

---

## 3. MikanLoader 相关路径（概念）

```
edk2/                          ← clone tianocore/edk2
  MdePkg/
  MikanLoaderPkg/              ← ln -s 链到 mikanos 仓库
    MikanLoader.inf
    Main.c
  …
Build/MikanLoaderX64/…/Loader.efi   ← build 产出
```

→ 官方流程 [mikanos-build](https://github.com/uchan-nos/mikanos-build) · [SETUP.md](../SETUP.md)

---

## 4. 待深入（随学习补充）

- [ ] `.inf` 字段：`SOURCES` · `LIBRARY_CLASSES` · `ENTRY_POINT`
- [ ] `.dsc` 中 `TOOL_CHAIN_TAG`（如 **CLANGPDB**）
- [ ] `build -p MikanLoaderPkg/MikanLoader.dsc -a X64`

---

← [Ch2 §2](../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md) · [附录 C 导读](../README.md)
