## ③ 编译和安装内核 · Building and Installing the Kernel

#### 配置

编译前 **必须配置**（生成 `.config`）：

| 命令 | 界面 |
|------|------|
| `make config` | 命令行逐项问答 |
| `make menuconfig` | **ncurses** 图形菜单（最常用） |
| `make defconfig` | 当前架构 **默认配置** 起点 |

还可 `make oldconfig` 在升级版本时合并新旧选项。

#### 编译

```bash
make -j$(nproc)    # 或 make -j8；通常 核心数 ~ 2× 核心
```

| 产物 | 说明 |
|------|------|
| **`vmlinuz` / `bzImage` 等** | 引导用内核镜像（因架构/引导器而异） |
| **`*.ko`** | 可加载 **内核模块** |

#### 安装

| 步骤 | 命令 / 位置 |
|------|-------------|
| **内核镜像** | 依 **架构 + GRUB/systemd-boot** 手动拷贝并改引导项 |
| **模块** | `make modules_install` → **`/lib/modules/<version>/`** |

**HFT 对照：** 调 **网卡驱动、PREEMPT、HZ、隔离 CPU** 往往从 **自建内核 + `menuconfig`** 开始；生产环境务必保留可回滚的已知-good 内核条目。

→ 课程前置：[01 LFS](../../../03-Linux-Kernel-Development/01_Course_LFS/)

---
