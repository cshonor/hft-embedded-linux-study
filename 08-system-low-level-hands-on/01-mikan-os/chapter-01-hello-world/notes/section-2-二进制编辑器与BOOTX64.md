## 2. C + Makefile 生成 BOOTX64.EFI

> **实际开发路径：** 你 **不必从零手写整个 EFI 文件**。MikanOS 用 **C 语言 + 交叉工具链**（Ch2 起再接入完整 **EDK II** 框架）生成符合规范的 **64 位 PE/COFF** 程序，放到 **FAT 镜像** 里即可被 UEFI 固件加载。
>
> **动手入口：** [code/](../code/) — `make` 一键编译出第一个 `BOOTX64.EFI`，`make run` 在 QEMU+OVMF 里看 Hello World。先跑通流程，再慢慢拆解 **ConOut / SystemTable** 等 UEFI 调用。
>
> **一句话：** **`.efi` 就是 C 语言写的 UEFI 程序，经交叉编译器编译链接后的产物** — 你写业务逻辑（初始化屏幕、打印、日后加载内核），**PE 头、节区、入口点** 由 Makefile / 工具链代劳，不必手写链接脚本。

---

### 一、EFI 是什么：C 源码 → 交叉编译 → 可执行文件

| 层次 | 你关心的 | 工具链代劳的 |
|------|----------|--------------|
| **源码** | `hello.c` — `EfiMain`、调 `ConOut` 打印、日后加载内核 | — |
| **编译** | 选对接口与类型（`SystemTable` 等） | `.c` → **COFF 对象** `.o` |
| **链接** | 指定入口符号名 `EfiMain` | 对象 → **PE32+** 格式的 **`BOOTX64.EFI`** |
| **部署** | 放进 `EFI/BOOT/` on FAT | UEFI 固件识别并加载 |

**MikanOS Ch1 入门示例** 就是几百行以内的 **最简 C UEFI 应用** — [code/hello.c](../code/hello.c) 只声明本章用到的结构体；**敲 `make` 即得 `bootX64.efi`**，不用碰底层 PE 字节布局（格式细节见 [§6](./section-6-C语言过渡与文件格式.md)）。

---

### 二、两套工具链怎么理解（上手版）

**先选一套装好就行。** Ch1 推荐 **纯 LLVM（Clang + ld.lld）**，WSL/Linux 下 `apt install llvm lld`（或 `clang lld`）即可 — **不必依赖 GCC 交叉链**，还能提前熟悉 LLVM 生态。

#### A. Clang + LLD — **LLVM 工具链组合**（**Ch1 首选**）

```
hello.c
  ↓  Clang          编译：C → 汇编(.s) → 目标文件(.o)
hello.o
  ↓  LLD 链接器     把 .o 拼成最终镜像
BOOTX64.EFI（PE/COFF，UEFI 可加载）
```

**两条 LLVM 子路径（都能出 EFI，二选一）：**

| 路径 | 编译 | 链接 PE/EFI |
|------|------|-------------|
| **A1 · Windows 原生** | `clang -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar -c` | **`lld-link /subsystem:efi_application …`** |
| **A2 · WSL/Linux** | `clang --target=x86_64-elf -ffreestanding -fshort-wchar -c` | **`ld.lld -flavor link -subsystem:efi_application …`** |
| **A3 · 官方 day01（WSL）** | `clang -target x86_64-pc-win32-coff -c` | **`lld-link`** — `make` 默认 |

**Windows 原生（PowerShell，在 `code\` 目录逐条执行）：**

```powershell
clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
New-Item -ItemType Directory -Force -Path esp\EFI\BOOT | Out-Null
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

→ 安装 LLVM、QEMU 完整步骤 [SETUP.md](../../SETUP.md)

```bash
clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
ld.lld -flavor link -subsystem:efi_application -entry:EfiMain hello.o -o bootX64.efi
```

| 组件 | 干什么 |
|------|--------|
| **Clang** | 前端：C → `.o`；`-ffreestanding` = 不依赖 libc，适合 UEFI/内核 |
| **ld.lld** | 同一 LLVM 链接器；**`-flavor link`** 切到 PE/COFF 模式链 **EFI**；链 **ELF 内核** 时不加 `-flavor link` |

**为何 HFT 也值得关注 LLVM 线：** 编译/链接迭代快、**Ch1 EFI → Ch4 内核 → EDK II 可统一 Clang** — 低延迟工程的 Release 构建循环更顺。

**环境：Windows 原生 LLVM 与 WSL/Linux 均可；流程一致，不必强依赖 WSL。**

**Windows 原生（PowerShell）：** [SETUP.md](../../SETUP.md) — `winget install LLVM.LLVM` 或官网安装包，勾选 PATH，然后：

```powershell
clang --target=x86_64-elf -ffreestanding -fshort-wchar -c hello.c -o hello.o
lld-link /subsystem:efi_application /entry:EfiMain hello.o /out:esp\EFI\BOOT\BOOTX64.EFI
```

**WSL / Linux 快速运行：**

```bash
sudo apt install -y llvm lld qemu-system-x86 ovmf   # 或 clang lld
cd chapter-01-hello-world/code
make LINK=ld.lld run    # 纯 LLVM；或 make run（默认 lld-link 路径）
```

#### B. x86_64-elf-gcc — **GCC 交叉编译链**（**可选 · 跟官方 build.sh**）

| 项 | 说明 |
|----|------|
| **是什么** | 目标三元组 **`x86_64-elf-*`** 的 GCC/G++ |
| **用来干什么** | 官方 [mikanos-build](https://github.com/uchan-nos/mikanos-build) 默认内核 / 部分 Loader 构建 |
| **何时装** | 要跟官方 **`build.sh` 一字不差** 时再装；**纯 LLVM 路线可跳过** |
| **与 A2 区别** | A2 的 `--target=x86_64-elf` 是 **Clang 三元组**，不是这套 GCC 包 — 名字像，工具不同 |

#### C. EDK II `build` + **全程 LLVM**（Ch2 MikanLoader）

在 EDK II 工程里把 **`TOOL_CHAIN_TAG` 改为 `CLANGPDB`**（或构建时 `-t CLANGPDB`），Loader 可 **全程 Clang/LLVM**，不必绑 GCC 交叉链。

→ 详见 [Ch2 §2 CLANGPDB](../../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md#五全程-llvmedk-ii-与-clangpdb)

---

### 三、对照表 · 现在该用哪套？

| 工具链 | 典型命令 | 适用阶段 | 说明 |
|--------|----------|----------|------|
| **Clang + ld.lld** | `--target=x86_64-elf -ffreestanding` · `ld.lld -flavor link …` | Ch1 · **纯 LLVM 推荐** | `make LINK=ld.lld`；**不依赖 GCC 交叉链** |
| **Clang + lld-link** | `-target x86_64-pc-win32-coff` · `lld-link` | Ch1 · 官方 [day01/c](https://github.com/uchan-nos/mikanos-build/tree/master/day01/c) | `make` 默认 |
| **x86_64-elf-gcc** | 官方 mikanos-build 包 | 跟官方 `build.sh` 时 | 与 Clang A2 **不是同一套工具** |
| **EDK II + CLANGPDB** | `build -t CLANGPDB` | Ch2 **MikanLoader** | EDK II 全程 LLVM |

**怎么选（一句话）：**

- **今天：** `apt install llvm lld` → **`make LINK=ld.lld run`** — 纯 LLVM 跑通 EFI。
- **跟官方 day01 一字不差：** `make run`（lld-link 路径）。
- **Ch2 EDK II：** `TOOL_CHAIN_TAG = CLANGPDB` — Loader 也走 LLVM。
- **GCC 交叉链：** 仅在与官方 **buildenv.sh** 绑定时需要，**非 LLVM 路线必选项**。

**覆盖编译器路径（示例）：**

```bash
make CLANG=/usr/bin/clang-18
# x86_64-elf-gcc --version   # 已 source buildenv.sh 后
```

**你不需要：** 自写 **链接脚本（.ld）** 拼 PE 头 — Makefile 里 **`/subsystem:efi_application` + `/entry:EfiMain`** 或官方 `build.sh` 已封装。

---

### 四、推荐流程（四步）

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

### 五、Makefile 一键编译

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

### 六、部署与引导测试

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

### 七、原书可选：二进制编辑器（概念体感）

原书 **第一节** 还演示用 **二进制编辑器** 直接填机器码生成 `BOOTX64.EFI` — 目的是看见「程序 = 文件里的字节序列」、理解 **PE 头比 512B IPL 复杂**。

| | **二进制编辑器（原书演示）** | **C + Makefile（实际开发）** |
|---|------------------------------|------------------------------|
| 目的 | 破除神秘感 · 感受字节与启动 | **可维护** 的日常路径 |
| 产出 | 手写 hex → `.efi` | `hello.c` → 工具链 → `.efi` |
| 是否必做 | **可选** — 时间紧可跳过 | **推荐** — 与本章 [code/](../code/) 对齐 |

→ 十六进制读法可参考 [02 HELLOOS_HEX_REFERENCE](../../02-30days-os/HELLOOS_HEX_REFERENCE.md)（软盘 IPL 不同，读 hex 方法相同）

---

### 八、常见初坑

| 现象 | 可能原因 |
|------|----------|
| `make` 找不到 `lld-link` | 未装 `lld` 包；或 WSL 外未配置 PATH |
| U 盘 / QEMU 无输出 | 路径非 `/EFI/BOOT/BOOTX64.EFI`、非 FAT、或 **Secure Boot** 拦截 |
| 乱码 / 崩溃 | `OutputString` 用了窄字符串；UEFI 文本 API 要 **`L"..."` 宽串** |
| 用宿主机 `gcc hello.c` 直接编 | 产出 **Linux ELF**，UEFI **不能加载** — 必须 **交叉** 到 PE/COFF（Clang `-target …-win32-coff` 或 MikanOS `x86_64-elf-*` 工具链） |

| 与软盘启动混淆 | 本章是 **FAT 上的 .efi 文件**，不是软盘 **512B IPL** — [§1.四](./section-1-本章定位.md#四核心区分bootx64efi--软盘启动两条线) |

**口述巩固 · 自测**

1. **`.efi` 和 `.c` 是什么关系？** — 同一份程序：C 写逻辑，交叉编译链接成 UEFI 可执行的 PE 文件。
2. **Ch1 纯 LLVM 怎么编？** — `clang --target=x86_64-elf -ffreestanding -c` + `ld.lld -flavor link -subsystem:efi_application -entry:EfiMain`；或 **`make LINK=ld.lld`**。
3. **lld-link 和 ld.lld？** — 同属 LLD；**`-flavor link`** 时 **ld.lld 也能链 EFI/PE**，不必绑 GCC。
4. **EDK II 全程 LLVM？** — **`TOOL_CHAIN_TAG = CLANGPDB`**（或 `build -t CLANGPDB`）。

---

← [1. 本章定位](./section-1-本章定位.md) · [code/ 动手](../code/) · 下一节 [3. 真机与 QEMU](./section-3-真机与QEMU测试.md)
