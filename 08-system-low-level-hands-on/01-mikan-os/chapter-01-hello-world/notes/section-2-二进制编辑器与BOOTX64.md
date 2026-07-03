## 2. C + Makefile 生成 BOOTX64.EFI

> **实际开发路径：** 你 **不必从零手写整个 EFI 文件**。MikanOS 用 **C 语言 + 交叉工具链**（Ch2 起再接入完整 **EDK II** 框架）生成符合规范的 **64 位 PE/COFF** 程序，放到 **FAT 镜像** 里即可被 UEFI 固件加载。
>
> **动手入口：** [code/](../code/) — `make` 一键编译出第一个 `BOOTX64.EFI`，`make run` 在 QEMU+OVMF 里看 Hello World。先跑通流程，再慢慢拆解 **ConOut / SystemTable** 等 UEFI 调用。

---

### 一、推荐流程（四步）

```
① 写 C 源码（EfiMain · 初始化控制台 · 打印字符）
        ↓
② Makefile：clang 编译 .o → lld-link 链接成 PE 格式的 .efi
        ↓
③ 复制为 esp/EFI/BOOT/BOOTX64.EFI（FAT 目录布局）
        ↓
④ UEFI 固件加载执行 → 屏幕 Hello, world!
```

| 步骤 | 工具 / 产物 | 说明 |
|------|-------------|------|
| **源码** | `hello.c` | 入口 **`EfiMain(ImageHandle, SystemTable)`** — UEFI 应用标准入口，不是 `main(argc, argv)` |
| **编译** | `clang -target x86_64-pc-win32-coff` | **交叉编译** 为 Windows COFF 对象 — x64 UEFI 应用底层即 **PE32+** |
| **链接** | `lld-link /subsystem:efi_application /entry:EfiMain` | 生成 **PE 格式** 的 `.efi` |
| **部署** | `EFI/BOOT/BOOTX64.EFI` on **FAT** | 固件按固定路径查找（见 [§3 测试](./section-3-真机与QEMU测试.md)） |

**最小模板核心逻辑**（[code/hello.c](../code/hello.c)）：

```c
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello, world!\n");
  while (1) { }
  return 0;
}
```

| 调用 | 作用 |
|------|------|
| **`SystemTable->ConOut`** | 固件提供的 **简单文本输出协议** — 本章「初始化控制台、打印字符」就靠它 |
| **`OutputString(..., L"...")`** | 宽字符串输出；`L` 前缀 = **UCS-2 / UTF-16 码元**（见 [§4 编码](./section-4-计算机结构与编码.md)） |

Ch1 模板 **只声明用到的结构体字段**，避免拉入整个 EDK II；Ch2 起改用 **`<Uefi.h>`** 与 **MikanLoader** 工程化构建 → [chapter-02](../../chapter-02-edk2-memmap/)。

---

### 二、Makefile 一键编译

在 [code/](../code/) 目录（WSL，已装 `clang` · `lld` · `qemu-system-x86_64` · `ovmf`）：

```bash
cd chapter-01-hello-world/code
make        # → esp/EFI/BOOT/BOOTX64.EFI
make run    # → QEMU + OVMF，fat:rw:esp 挂载
```

**Makefile 要点**（与官方 [mikanos-build/day01/c](https://github.com/uchan-nos/mikanos-build/tree/master/day01/c) 一致）：

```makefile
hello.o: hello.c
	clang -target x86_64-pc-win32-coff -o $@ -c $<

esp/EFI/BOOT/BOOTX64.EFI: hello.o
	lld-link /subsystem:efi_application /entry:EfiMain /out:$@ $<
```

| 目标 | 含义 |
|------|------|
| **`x86_64-pc-win32-coff`** | 告诉 Clang：产出 **PE/COFF** 对象，不是 Linux ELF |
| **`/subsystem:efi_application`** | 链接器标记为 **UEFI 应用**，固件才认 |
| **`/entry:EfiMain`** | 入口符号 = UEFI 加载后第一条执行的 C 函数 |

完整 devenv（`make_image.sh` · 实体 U 盘镜像）→ [SETUP.md](../../SETUP.md) · [appendix-B](../../appendix-B-get-mikanos/)。

---

### 三、部署与引导测试

```
FAT 格式卷（U 盘或 QEMU fat:rw: 目录）
└── EFI/
    └── BOOT/
        └── BOOTX64.EFI    ← make 产出，或手动复制 hello.efi 并重命名
```

| 要点 | 说明 |
|------|------|
| **文件系统** | **FAT32**（常见 U 盘 / ESP 分区格式） |
| **路径固定** | UEFI 固件查找 **`/EFI/BOOT/BOOTX64.EFI`** |
| **架构** | **x86-64** — 文件名 `X64` 表示 64 位 EFI |

→ 真机 Secure Boot · QEMU 命令见 [§3](./section-3-真机与QEMU测试.md)

---

### 四、原书可选：二进制编辑器（概念体感）

原书 **第一节** 还演示用 **二进制编辑器** 直接填机器码生成 `BOOTX64.EFI` — 目的是看见「程序 = 文件里的字节序列」、理解 **PE 头比 512B IPL 复杂**。

| | **二进制编辑器（原书演示）** | **C + Makefile（实际开发）** |
|---|------------------------------|------------------------------|
| 目的 | 破除神秘感 · 感受字节与启动 | **可维护** 的日常路径 |
| 产出 | 手写 hex → `.efi` | `hello.c` → 工具链 → `.efi` |
| 是否必做 | **可选** — 时间紧可跳过 | **推荐** — 与本章 [code/](../code/) 对齐 |

→ 十六进制读法可参考 [02 HELLOOS_HEX_REFERENCE](../../02-30days-os/HELLOOS_HEX_REFERENCE.md)（软盘 IPL 不同，读 hex 方法相同）

---

### 五、常见初坑

| 现象 | 可能原因 |
|------|----------|
| `make` 找不到 `lld-link` | 未装 `lld` 包；或 WSL 外未配置 PATH |
| U 盘 / QEMU 无输出 | 路径非 `/EFI/BOOT/BOOTX64.EFI`、非 FAT、或 **Secure Boot** 拦截 |
| 乱码 / 崩溃 | `OutputString` 用了窄字符串；UEFI 文本 API 要 **`L"..."` 宽串** |
| 与软盘启动混淆 | 本章是 **FAT 上的 .efi 文件**，不是软盘 **512B IPL** — [§1.四](./section-1-本章定位.md#四核心区分bootx64efi--软盘启动两条线) |

**学习顺序建议：** 先 **`make run` 跑通** → 再读 `hello.c` 里每一行 UEFI 调用 → Ch2 接入 EDK II 后对照 `<Uefi.h>` 里的完整类型定义。

---

← [1. 本章定位](./section-1-本章定位.md) · [code/ 动手](../code/) · 下一节 [3. 真机与 QEMU](./section-3-真机与QEMU测试.md)
