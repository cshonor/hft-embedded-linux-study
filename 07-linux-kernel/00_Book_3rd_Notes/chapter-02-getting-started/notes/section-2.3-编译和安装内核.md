## ③ 编译和安装内核 · Building and Installing the Kernel

下载源码之后，**配置 → 编译 → 安装** 是关键步骤。流程不复杂，但 **环境必须对**；报错先看日志，再逐项排。

> **对本机的硬提醒：**  
> `C:\Users\12392\Desktop\linux-7.1.5` 适合 **阅读对照 LKD**。  
> **不要在纯 Windows 里对这棵树执行 `make` 当启动内核用** — 需要 **Linux 本机 / WSL2 / 虚拟机 / 交叉编译到板子**。  
> 学习 Phase：现在 Phase1–2 以读源码为主；**真编译** 建议放到 Phase4（LKD）或嵌入式 `11-embedded-boot-build` 时再上机。

---

### 一、整体流水线

```
装编译工具（gcc make 等）
        │
        ▼
进入源码目录（用户目录下的 linux-x.y.z）
        │
        ▼
配置内核（menuconfig / defconfig …）→ 生成 .config
        │
        ▼
make -jN 编译（耗时长，正常）
        │
        ▼
make modules_install  （装模块）
make install          （装镜像+引导项，视发行版）
        │
        ▼
更新引导（grub 等）→ 重启选新内核（务必留旧项可回滚）
```

遇错别慌：终端 **第一处 error:** / 缺头文件 / 缺包名，通常就指向原因。

---

### 二、准备：编译工具链

在 **Debian/Ubuntu 类** 示例（包名因发行版略有差异）：

```bash
sudo apt update
sudo apt install -y build-essential bc bison flex libssl-dev libelf-dev \
  libncurses-dev dwarves pahole
# menuconfig 需要 ncurses；部分配置还要 bison/flex、openssl、elf
```

| 组件 | 作用 |
|------|------|
| **gcc** / **g++** | 编译内核与模块 |
| **make** | 驱动 Kbuild |
| **bc / bison / flex** | 部分生成/脚本依赖 |
| **libncurses-dev** | `make menuconfig` |
| **libssl-dev / libelf-dev** | 签名、BTF 等现代选项常要 |

嵌入式交叉编译另装 `aarch64-linux-gnu-gcc` 等 — 见 `11-embedded-boot-build`，此处先讲 **本机架构原生编译**。

---

### 三、进入源码目录

```bash
cd ~/linux-7.1.5          # Linux/WSL：用户目录，勿用 /usr/src/linux
# 或把 Desktop 树拷进 WSL：
# cp -a /mnt/c/Users/12392/Desktop/linux-7.1.5 ~/
# cd ~/linux-7.1.5
```

| 规则 | 原因 |
|------|------|
| 在用户目录编译 | 不必（也不应）污染系统源码树 |
| 磁盘预留数 GB～十几 GB | 全量编译产物很大 |

---

### 四、配置（必须先做）

编译前要有 **`.config`**：

| 命令 | 界面 / 用途 |
|------|-------------|
| **`make menuconfig`** | ncurses 菜单 — **最常用** |
| `make config` | 命令行逐项问答 — 慢 |
| `make defconfig` | 当前架构默认配置 — 快速起点 |
| `make oldconfig` | 升级源码后合并新旧选项 |
| `make localmodconfig` | 按当前机已加载模块裁剪 — 笔记本试用友好 |

```bash
make defconfig          # 或直接
make menuconfig         # 改完 Save → Exit
```

**HFT / 低延迟相关选项（知道在哪改即可，勿一上来乱关驱动）：**  
抢占模型、HZ、无用驱动裁剪、网卡相关、cgroup/命名空间等 — 生产改内核要有回滚盘。

---

### 五、编译

```bash
make -j"$(nproc)"       # 并行；也可用 make -j8
# 只要模块：
# make modules -j"$(nproc)"
```

| 产物 | 说明 |
|------|------|
| **`arch/*/boot/bzImage`** 等 | 可引导镜像（架构不同名字不同） |
| **`*.ko`** | 可加载模块 |
| 时间 | 全量可能 **数十分钟～数小时** — 属正常 |

只想验证工具链通了，可先：

```bash
make -j"$(nproc)" net/core/dev.o    # 单目标，快失败快反馈
```

---

### 六、安装

| 步骤 | 命令 | 说明 |
|------|------|------|
| 装模块 | `sudo make modules_install` | 写入 `/lib/modules/<version>/` |
| 装内核 | `sudo make install` | 许多发行版会拷镜像并改 grub；**不是所有架构/发行版行为都一样** |
| 手动路径 | 拷 `bzImage` + `System.map` + initramfs，改引导项 | 嵌入式/定制根更常见 |

```bash
sudo make modules_install
sudo make install          # 发行版支持时
# 然后按发行版更新 grub / 生成 initramfs，再 reboot
```

| 安全习惯 | |
|----------|--|
| **保留旧内核引导项** | 新内核起不来还能选回去 |
| 先在 **虚拟机** 练手 | 勿直接拿唯一的工作机当小白鼠 |
| 模块与运行中内核版本必须匹配 | `uname -r` ↔ `/lib/modules/...` |

书中「安装」偏通用描述；**真机以你的发行版文档为准**（Ubuntu/Fedora/Arch 步骤不同）。

---

### 七、报错怎么处理

| 心态 | 做法 |
|------|------|
| 别慌 | 向上翻 **第一条 error**，不是最后一堆 warning |
| 缺包 | `xxx.h: No such file` → 装对应 `-dev` 包 |
| 磁盘满 | `No space left` → 清 `*.o` 或换盘 |
| 权限 | `Permission denied` 写 `/lib` `/boot` → 用 `sudo` 且确认路径 |
| 配置矛盾 | menuconfig 里依赖没满足 → 按提示打开依赖选项 |

把 **完整报错段**（含上面 20 行）贴出来就能继续往下排 — 这是正常协作方式。

---

### 八、和本仓库其它入口

| 入口 | 用途 |
|------|------|
| [§2.1 获取源码](./section-2.1-获取内核源码.md) | 版本 7.1.5 / 2.6.34、下载验收 |
| [P3.5 BusyBox 极简 Linux](../../../../projects/P3.5-busybox-minimal-linux/) | 从零构建语境下的内核编译课 |
| [11-embedded-boot-build](../../../../11-embedded-boot-build/) | 嵌入式构建、交叉编译 |
| [§2.2 源码树](./section-2.2-内核源码树.md) | 编译前先认目录 |

**经验问题答一句：** 工具侧可以陪你从 `menuconfig` 到排错；你这边关键是 **准备好 Linux/WSL 环境**，在 Windows 上把 7.1.5 当「只读教科书」即可。真要第一次全量编译，建议 **Ubuntu 虚拟机** 里对拷过去的 `linux-7.1.5` 跑通 `defconfig → make -j`，成功后再谈 `modules_install`。

编译产物如何被 UEFI/GRUB 加载、为何用户态是 ELF → [§2.5](./section-2.5-ELF与UEFI启动链路.md)

---
