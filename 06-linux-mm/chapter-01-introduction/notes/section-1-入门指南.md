# Ch 1 §1 入门指南 (Getting Started)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（顶层 `Makefile` / `Documentation/process/`）

---

## 本节讲什么

本节回答三个问题：

1. 拿到内核源码后，「配置 → 编译 → 出镜像」这条链在现代（v6.6）到底怎么走？
2. `vmlinux`、`bzImage`、`Image` 这些编译产物**各自是什么、区别在哪**？
3. 有哪些**信息获取渠道**是内核开发者真正在用的？

原书（2.4/2.6 时代）的 `make config` / `make bzImage` 命令名已经过时，但「**先配置、再编内核+模块**」这条主流程 20 年没变。本节把命令**对齐到 v6.6**。

---

## 1. 配置：从 `.config` 到 autoconf

内核一切功能开关都收敛到一个文件：**`.config`**。它不是给编译器看的，而是**给 Kbuild 生成头文件用的输入**。

```bash
make menuconfig      # 交互式 ncurses 菜单，最常用
make olddefconfig    # 用现有 .config，新符号取默认值（脚本化/升级用）
make defconfig       # 架构默认配置（arch/x86/configs/x86_64_defconfig）
make localmodconfig  # 只启用当前已加载的模块（快速构建/嵌入式瘦身神器）
make allyesconfig    # 全开（压测/API 覆盖率用，别拿来跑生产）
```

`make *config` 的实际动作（`scripts/kconfig/`）：

```
.config ──> scripts/kconfig/conf ──> include/generated/autoconf.h   （#define CONFIG_XXX）
                                  ──> include/config/auto.conf       （Makefile 用）
                                  ──> include/config/                （每个符号一个空文件，依赖追踪）
```

关键直觉：**`CONFIG_XXX` 不是魔法，就是 `autoconf.h` 里的 `#define`**。源码里 `#ifdef CONFIG_HIGHMEM` 之所以能在 x86_64 上消失，就是因为 x86_64 的 defconfig 没开这项 → `autoconf.h` 里没有这个宏（Ch2 §4 高端内存那节的实证根源就在这）。

---

## 2. 编译：`vmlinux` vs `bzImage` vs `Image`

这是最容易被一句「`make bzImage`」带过、但其实最该搞清楚的地方。**产物层级**：

| 产物 | 是什么 | 格式 | 谁来用 |
|------|--------|------|--------|
| `vmlinux` | **未压缩的完整内核镜像** | ELF（带符号表，可 `gdb`/`objdump`/`nm`） | 调试、反汇编、`crash` 分析 |
| `vmlinux.bin` | `vmlinux` 去掉 ELF 头/符号的**裸二进制** | raw binary | `objcopy` 中间产物 |
| `bzImage`（x86） | **自解压的引导镜像** = `setup.bin` + 压缩内核 | 引导器可加载 | 交给 bootloader / `qemu -kernel` |
| `Image`（arm64） | 未压缩裸内核 | raw binary | arm64 引导（无自解压） |
| `zImage`/`uImage`（arm） | 压缩镜像 / 带 u-boot 头 | 引导器可加载 | 树莓派等老 arm |

**`bzImage` 的生成链**（`arch/x86/boot/Makefile`）：

```
vmlinux
  └─ objcopy → vmlinux.bin                  # 去 ELF 头
        └─ gzip   → vmlinux.bin.gz          # 压缩
              └─ 嵌入 piggy.S → compressed/vmlinux   # 自解压 stub 把压缩内核"背"在身上
                    └─ + setup.bin（实模式 setup 代码）
                          └─ bzImage        # 引导器只需加载这一个文件
```

「bz」= **b**ig **z**Image（突破早期 zImage 的 512KB 上限，用**分段加载**）。bootloader 加载 bzImage 后，CPU 先跑实模式 `setup.bin`（探测内存、进保护模式），再跳到保护模式下的自解压 stub，把 `vmlinux.bin.gz` 解到 `_text` 处开跑。

**为什么 `vmlinux` 要单独留一份？** 因为它带符号表。`crash` / `gdb vmlinux` / `objdump -dS` 全靠它定位函数和行号——**读 `mm/` 源码、追 oops 时的 `EIP` 都要先有 vmlinux**。这正是 Ch1 全书方法论（读代码、跟调用链）的物理前提。

```bash
make -j$(nproc)         # 并行编译（默认目标 = vmlinux + modules）
make bzImage            # 只编引导镜像
make modules            # 只编可加载模块（.ko）
make modules_install    # 装到 /lib/modules/$(uname -r)/
make install            # 装内核 + initramfs + 更新引导
```

---

## 3. 交叉编译与嵌入式瘦身

嵌入式（树莓派 5 / 各种 SoC）和 HFT 网关机的两条实用路径：

```bash
# 交叉编译（x86 宿主机 → aarch64 目标）
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig   # 树莓派用 bcm2712_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image modules

# 瘦身：只保留"当前机器正在用的"模块，编译时间从小时级降到分钟级
make localmodconfig      # 读 lsmod，自动裁剪 CONFIG
```

**HFT 关联**：延迟敏感环境里，内核功能越少越省——少一个 `CONFIG_XXX` 就少一段初始化、少一个可能抢 CPU 的中断/定时器。但**剪配置要克制**：`localmodconfig` 会按「当前插着哪些硬件」裁剪，换网卡/换 CPU 后可能缺驱动。生产机的正确姿势是**固化一份最小 defconfig 进版本库**，而不是每次 `localmodconfig`。

---

## 4. 信息获取渠道

| 资源 | 定位 | 用途 |
|------|------|------|
| 源码顶层 `README` | 版本、最简编译提示 | 快速确认"这棵树能不能编" |
| `Documentation/process/` | 官方流程文档（`coding-style.rst`、`submitting-patches.rst`） | 提交补丁前必读（§5） |
| `Documentation/` 其余 | 各子系统文档（含 `admin-guide/mm/`） | 查 NUMA / THP / swap 的行为语义 |
| [LWN.net](https://lwn.net/) | 深度技术文 + 内核动向 | **mm 方向必读**，很多内核设计动机只有 LWN 讲得透 |
| Kernelnewbies | 新手 FAQ、术语表 | 查术语、查历史 |
| LKML | 补丁讨论主战场 | 看某个补丁**为什么被拒/怎么改** |
| [lore.kernel.org](https://lore.kernel.org/) | LKML 归档（现代替代） | 按 Message-ID 查完整讨论线程 |

> 原书提的 **Kernel Traffic** 早已停更，现代等价物是 **LWN + lore.kernel.org 归档**——这条要更新认知，别去搜 Kernel Traffic。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 追 oops 栈回溯 | 需要带符号的 `vmlinux` 才能把地址翻译成函数+行号 |
| 网关机启动要快、内存要省 | `localmodconfig` 裁剪 + 固化最小 defconfig |
| 交叉编译到 arm64 网关/采集卡 | `CROSS_COMPILE=aarch64-linux-gnu-` + `Image`（非 x86 的 bzImage） |
| 读 `mm/` 源码前 | 先有个能编译的树 + vmlinux + Elixir（§3），缺一不可 |

---

## 6. 衔接

- 下节 [§2 源码管理](./section-2-源码管理.md)：补丁怎么生成、怎么管理
- [§3 浏览代码](./section-3-浏览代码.md)：拿什么工具读这棵编译好的树
- 高端内存实证：[Ch2 §4 高端内存](../../chapter-02-describing-physical-memory/notes/section-4-高端内存.md)（`CONFIG_HIGHMEM` 由 defconfig 决定）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`make olddefconfig` 和 `make defconfig` 差在哪？**
A：`defconfig` 从**架构默认配置**开始（忽略你当前的 `.config`）；`olddefconfig` 基于**你现有的 `.config`**，只对新出现的 CONFIG 符号取默认值。升级内核版本时用 `olddefconfig` 能**保留你之前的所有定制**，用 `defconfig` 会全部丢掉。

**Q2：`vmlinux` 和 `bzImage` 到底哪个能直接跑？**
A：都「能跑」，但场景不同。`bzImage` 是**引导器能加载**的格式（自解压 + setup 代码），`qemu -kernel bzImage` 直接起。`vmlinux` 是**完整 ELF**，引导器不认，但它是**调试的根**——`gdb`、`crash`、`objdump` 都用它。一句话：**跑用 bzImage，看用 vmlinux**。

**Q3：为什么 `localmodconfig` 能大幅缩短编译时间？**
A：普通 `defconfig` 几乎全开，会编进海量**你永远用不到的驱动**（几百个网卡、声卡、文件系统模块）。`localmodconfig` 读 `/proc/modules`（`lsmod` 的数据源），只保留「当前已加载」的模块对应的 CONFIG，把几千个模块砍到几十个，编译量断崖式下降。

**Q4：`.config` 里改一个符号，重新 `make` 会怎样？**
A：Kbuild 用 `include/config/` 下每个符号一个空文件做**依赖追踪**，`.config` 变了会**只重编受影响的目标**（增量编译），不是全量重来。但如果改了影响面极大的符号（如 `CONFIG_SMP`），几乎所有文件都会重编——这就是为什么改这类符号后要预留编译时间。

**Q5：嵌入式交叉编译为什么要 `ARCH=arm64` 而不是直接 `make`？**
A：`ARCH` 决定 Kbuild 进哪个 `arch/` 目录、用哪套 `arch/arm64/Makefile` 规则和 defconfig；`CROSS_COMPILE` 前缀把工具链从 `gcc` 换成 `aarch64-linux-gnu-gcc`。漏了任何一个，都会编出「宿主机的 x86 内核」而不是目标板能跑的内核——这错误新手极常见。

</details>

---
