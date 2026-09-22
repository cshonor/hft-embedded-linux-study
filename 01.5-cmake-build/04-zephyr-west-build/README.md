# 第 4 章 · 解剖 `west build`：Zephyr 里的 CMake

> **本节讲什么：** 前三章的 CMake 是自己写 CMakeLists。这章换个视角：
> 看一个**工业级项目**（Zephyr RTOS）怎么把同一套机制用到几百个模块的规模。
> 一句 `west build` 背后：west → cmake 的调用链、Kconfig 怎么决定哪些源文件进构建、
> 以及它和 Linux 内核 Kbuild 那对"表兄弟"的逐项对照。
> 这章是**读代码课**——demo/ 是一个最小 Zephyr 应用的标本，目标是"看得懂"，不要求有板子。
>
> 前置：第 1、3 章（target / add_subdirectory / target_sources）。
> 说明：输出示例取自 Zephyr 3.x 构建日志的标准格式。

---

## 4.1 问题的起点：一句 `west build`，谁干的活？

Zephyr 工程的构建入口长得和 CMake 毫无关系：

```bash
west build -b stm32f4_disco samples/basic/blinky
```

没有 `cmake -B build`，没有 toolchain file 参数。但 `west build -v`（verbose）
的第一行就露底了：

```text
-- west build: generating a build system
Running: cmake -B build -S samples/basic/blinky -GNinja
         -DBOARD=stm32f4_disco -DCMAKE_TOOLCHAIN_FILE=.../zephyr/cmake/toolchain/...
```

**west 没有取代 CMake，它只是替你把前三章学的参数全都拼好**：build 目录、源码目录、
生成器（Zephyr 默认 Ninja 而不是 Make）、板子型号、toolchain file。

分工一句话：**west 是项目经理（管参数、管多个仓库），CMake 还是施工队**。

| 工具 | 角色 | 前三章的对应物 |
|---|---|---|
| `west` | 拼参数 + 调用 cmake；另管 Zephyr 生态的多仓库（`west update` 拉几十个依赖仓库） | 你手动敲的 `cmake -B build -D...` |
| `cmake` | 真正的配置与构建生成 | 第 1 章的两条命令，一字未变 |
| `ninja` | 生成的构建系统（Zephyr 选它因为大工程下比 make 快） | 第 1 章 build/ 里的 Makefile，换成 build.ninja |

验证：`west build` 之后进 `build/` 目录，`build.ninja`、`CMakeCache.txt` 都在——
第 1 章"拆开 build/"的方法原样适用。

## 4.2 应用的 CMakeLists 为什么长得不一样

demo/CMakeLists.txt 里有两个"反直觉"，对照普通 CMake 工程看：

```cmake
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})   # ① 在 project() 之前
project(blinky_demo C)
target_sources(app PRIVATE src/main.c)                   # ② 没有 add_executable
```

**① `find_package(Zephyr)` 必须在 `project()` 之前。**
它一口气完成：定位 Zephyr 内核源码树、根据 `BOARD` 选定板级配置、
注入 Zephyr 的 toolchain file（第 2 章那套，Zephyr 官方替你写好了）、
并把内核的构建逻辑挂进来。普通工程"project() 第一行"的铁律，
在 Zephyr 里被这个"总开关"顶到了第二行。

**② `app` target 是 Zephyr 建好的，应用只往里挂文件。**
`target_sources(app PRIVATE src/main.c)`——第 3 章"一切皆 target"的推论：
Zephyr 预先创建了 `app` 这个可执行 target 和一堆内核库 target
（`kernel`、`drivers`、`liblibc`……），应用的 CMakeLists 只描述
"我有哪些源文件、我开哪些配置"。规模再小也是生态的一员。

## 4.3 Kconfig ↔ CMake：配置怎么变成代码

Zephyr 应用目录里除了 CMakeLists 还有个 `prj.conf`（demo/ 里有标本）：

```text
CONFIG_GPIO=y
```

这一行要走完一条四站链路才真正生效：

```text
prj.conf                .config               autoconf.h              构建系统
CONFIG_GPIO=y  ──合并──▶  全量配置      ──翻译──▶ #define CONFIG_GPIO 1 ──决定──▶ drivers/gpio/*.c
（应用层覆盖）   （板级默认+应用覆盖）   （C 头文件）              是否编进内核 target
```

1. **合并**：板子的默认配置 + `prj.conf` + 命令行 `-DCONFIG_X=y`，合成完整 `.config`；
2. **翻译**：Kconfig 工具把 `.config` 生成 `autoconf.h`——每个 `CONFIG_X=y` 变 `#define CONFIG_X 1`；
3. **进代码**：C 源码里 `#if IS_ENABLED(CONFIG_GPIO)` 据此裁剪；
4. **进构建**：Zephyr 各子系统的 CMakeLists 按 CONFIG 决定 `target_sources` 加不加对应文件——
   `CONFIG_GPIO=n` 时 GPIO 驱动的一个字节都不会进固件。

这就是嵌入式"可裁剪"的实现：**同一个 RTOS，配置文件不同，编出来的内核完全不同**。

## 4.4 与 Linux 内核 Kbuild 的对照（05 书 ch4 的桥）

Zephyr 这套 Kconfig 是从 Linux 内核借来的，两边对照着看最清楚：

| | Linux 内核（Kbuild） | Zephyr（Kconfig + CMake） |
|---|---|---|
| 配置入口 | `make menuconfig` → `.config` | `prj.conf` + `west build -DCONFIG_X` → `.config` |
| 配置→头文件 | `include/generated/autoconf.h` | `build/zephyr/include/generated/autoconf.h`（同名同构） |
| 配置→源文件 | Makefile 里 `obj-$(CONFIG_GPIO) += gpio.o` | CMakeLists 里按 CONFIG 条件 `target_sources` |
| 构建系统 | 手写 Make 规则体系（Kbuild） | CMake 生成 Ninja |
| 模块机制 | `.ko` 可加载模块（05 书 ch4） | 无运行时模块，全靠编译期裁剪 |

一句话：**Kconfig 是同一条血脉（语法、autoconf.h 都一样），
"配置翻译成构建动作"的管道从 Makefile 换成了 CMake**。
05 书 ch4 学过 `obj-$(CONFIG_X)` 的，看到 Zephyr 的 `target_sources(... 按 CONFIG 条件)`
会有强烈的既视感——要的就是这个。

## 4.5 拆开 Zephyr 的 build/：比第 1 章多了什么

```text
build/
├── build.ninja            ← 生成的构建系统（第 1 章的 Makefile 对应物）
├── CMakeCache.txt         ← BOARD、ZEPHYR_BASE、toolchain 全在里面，可查
├── .config                ← Kconfig 合并结果（4.3 第 1 站）
├── zephyr/
│   ├── include/generated/autoconf.h   ← 4.3 第 2 站
│   ├── zephyr.elf / zephyr.bin        ← 最终固件（.bin 用来烧录）
│   └── zephyr.dts                     ← 预处理后的设备树（09 模块的主角）
└── modules/               ← 各依赖仓库（HAL 库、协议栈）的中间产物
```

排障路径和第 1 章完全一样：**配置对不对看 `.config`，代码里有没有看 `autoconf.h`，
编没编进固件看 `build.ninja` 里搜不搜得到那个源文件**。
三层各管一段，症状在哪层就去哪层查。

## 4.6 本章验收清单

- [ ] 说出 `west build` 实际执行了什么（cmake 命令 + 它拼的四个关键参数）
- [ ] 解释 Zephyr 应用 CMakeLists 的两个反直觉（find_package 前置、target_sources 挂 app）
- [ ] 背出 `prj.conf` → 固件的四站链路，知道每一站的文件在哪
- [ ] 填出 Kconfig↔CMake 与 Kconfig↔Kbuild 对照表的任意一行
- [ ] 给一个"CONFIG 开了但驱动没编进固件"的症状，说出三层排查路径

## 4.7 与后续衔接

- **09-device-drivers-dt**：设备树是 Zephyr/内核硬件描述的另一条输入线，
  本章只露了一面（`zephyr.dts`），驱动章正面展开
- **08-embedded-boot-build**：Buildroot 是"发行版级"的裁剪，Kconfig 是"内核级"的裁剪，
  两套思想同源
- 小练习：用 `west build -t menuconfig` 打开图形配置界面，把 `CONFIG_GPIO` 关掉再构建，
  到 `autoconf.h` 和 `build.ninja` 里各验证一次"它真的消失了"

## 代码自测

<details><summary>Q1：west 和 cmake 是什么关系？west build 之后还能用 cmake --build 吗？</summary>

west 是 Zephyr 生态的元工具：拼好 cmake 的全部参数（源码目录、build 目录、
BOARD、toolchain file、生成器）再调用 cmake，另外负责多仓库管理。
构建目录生成后就是普通 CMake build 目录——`cmake --build build` 照样可用，
增量构建时 west 内部调的也是它。类比：west 是"装修项目经理"，
真正干活的水电工还是 cmake + ninja。
</details>

<details><summary>Q2：为什么 Zephyr 应用用 target_sources(app ...) 而不是 add_executable？</summary>

因为固件这个"可执行文件"不只是应用的代码——它要链接 Zephyr 内核、驱动、
C 库、板级启动代码，链接脚本也由 Zephyr 按板子生成。Zephyr 预先创建好
app target 并把整套链接逻辑挂好，应用只声明"我的源文件有哪些"。
这是"一切皆 target"的规模化用法：框架提供骨架 target，用户往骨架上挂肉。
自己 add_executable 等于把内核构建逻辑全推给应用自己写。
</details>

<details><summary>Q3：prj.conf 里 CONFIG_GPIO=y 是怎么一步步影响到固件内容的？</summary>

四站：①合并——prj.conf 与板级默认、命令行 -D 合并成 build/.config；
②翻译——Kconfig 工具生成 autoconf.h，CONFIG_GPIO=y 变 #define CONFIG_GPIO 1；
③裁剪代码——C 源码的 IS_ENABLED(CONFIG_GPIO) 为真，相关代码保留；
④裁剪构建——GPIO 子系统的 CMakeLists 按 CONFIG 条件把驱动源文件加入 target，
编译进固件。CONFIG_GPIO=n 时第③④站同时生效：代码和编译单元双双消失。
</details>

<details><summary>Q4：Zephyr 的 Kconfig 和 Linux 内核的 Kconfig 是什么关系？</summary>

同一条血脉：Zephyr 直接借用了内核的 Kconfig 语言与工具（菜单语法、依赖表达式、
autoconf.h 生成机制都一样）。差别在"配置翻译成构建动作"的管道：内核用 Kbuild
（Makefile 里 obj-$(CONFIG_X) += x.o），Zephyr 用 CMake（按 CONFIG 条件
target_sources）。另外内核支持运行时加载模块（.ko），Zephyr 没有运行时模块，
一切裁剪都在编译期完成——更契合单片机资源受限、无动态加载的场景。
</details>

<details><summary>Q5：menuconfig 里明明开了某个驱动，固件里却没有，怎么排查？</summary>

三层排查法（各管一段，逐层确认）：①配置层——查 build/.config 里该 CONFIG 是否
真的 =y（可能被依赖条件 silently 关掉：Kconfig 的 depends on 不满足时选项会被强制回退）；
②头文件层——查 autoconf.h 里有没有对应 #define；③构建层——在 build.ninja 里
grep 驱动源文件名，确认它有没有编译规则。三层逐一定位：①没有→Kconfig 依赖问题；
①有②没有→翻译环节异常（罕见）；①②有③没有→该子系统 CMakeLists 的条件判断问题。
</details>
