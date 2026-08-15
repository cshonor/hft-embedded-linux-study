# 内核源码组织与编译

> 来源: Bootlin Kernel Training
> 对标旧书: ULK3 Ch1 (源码组织已过时)

---

## 内核源码目录树 (6.x)

```
linux-6.x/
├── arch/           # 架构相关 (arm64/, x86/, riscv/)
│   └── arm64/
│       ├── kernel/     # head.S, entry.S, 调度, 信号
│       ├── mm/         # 页表, TLB, cache flush
│       └── include/    # 架构特定头文件
├── kernel/         # 核心子系统 (sched/, fork.c, exit.c)
├── mm/             # 内存管理 (buddy, slab, page cache)
├── fs/             # 文件系统 (VFS + 具体文件系统)
├── block/          # 块 I/O 层 (blk-mq)
├── net/            # 网络栈 (socket, TCP/IP, netfilter)
├── drivers/        # 设备驱动 (数千个子目录)
├── include/        # 内核头文件 (linux/, asm-generic/)
├── Documentation/  # 内核文档 (RST 格式)
├── tools/          # 用户空间工具 (perf, bpftool, virtio)
├── scripts/        # 编译脚本 (Kconfig, Makefile)
├── init/           # 启动代码 (main.c: start_kernel)
├── lib/            # 内核通用库 (string, bitmap, rbtree)
├── security/       # 安全框架 (SELinux, AppArmor)
└── sound/          # 音频子系统
```

### 关键目录说明

| 目录 | 作用 | HFT 关注 |
|------|------|----------|
| `kernel/sched/` | 调度器 (fair.c = EEVDF) | EEVDF 调度策略 |
| `mm/` | 内存管理 | SLUB 分配器, page cache |
| `net/core/` | 网络核心 | socket buffer (sk_buff) |
| `drivers/net/ethernet/` | 网卡驱动 | ixgbe, mlx5, ena |
| `block/blk-mq.c` | 多队列块 I/O | 磁盘 I/O 延迟 |
| `arch/arm64/kernel/head.S` | ARM64 启动入口 | 启动流程 |
| `include/linux/sched.h` | task_struct 定义 | 进程结构体 |

---

## 编译内核 (树莓派 5 / ARM64)

```bash
# 1. 获取树莓派 5 内核源码
git clone --depth 1 https://github.com/raspberrypi/linux.git
cd linux
git checkout rpi-6.1.y

# 2. 生成编译配置
make ARCH=arm64 defconfig      # 默认配置
make ARCH=arm64 menuconfig     # 自定义配置

# 3. HFT 相关编译选项
# Kernel hacking → Memory debugging:
#   CONFIG_KASAN=y          (开发期, 生产关闭)
#   CONFIG_KFENCE=y         (轻量, 可生产)
#   CONFIG_DEBUG_INFO=y     (调试符号)
# General setup:
#   CONFIG_PREEMPT_RT=y     (实时内核)
#   CONFIG_HZ_1000=y        (高精度时钟)

# 4. 交叉编译 (在 x86 主机上编译 ARM64)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules

# 5. 安装到树莓派 SD 卡
sudo make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
    INSTALL_MOD_PATH=/mnt/sdcard modules_install
sudo cp arch/arm64/boot/Image /mnt/sdcard/boot/kernel8.img
sudo cp arch/arm64/boot/dts/broadcom/*.dtb /mnt/sdcard/boot/
```

### 编译产物

| 文件 | 说明 |
|------|------|
| `arch/arm64/boot/Image` | 内核镜像（未压缩） |
| `arch/arm64/boot/Image.gz` | 压缩内核镜像 |
| `*.dtb` | 设备树二进制 |
| `modules/*.ko` | 可加载内核模块 |

---

## 源码浏览工具

### Elixir 交叉索引

```bash
# 在线浏览: https://elixir.bootlin.com/linux/v6.1.63/source
# 支持符号跳转: 函数定义 → 调用者 → 被调用者

# 本地部署 Elixir
# 或使用 cscope/ctags
make ARCH=arm64 cscope    # 生成 cscope 索引
make ARCH=arm64 tags      # 生成 ctags 索引
```

### grep + git log 快速定位

```bash
# 查找函数定义
grep -rn "int schedule(void)" kernel/sched/

# 查找结构体定义
grep -rn "struct sched_entity {" include/linux/sched.h

# 查看某文件的修改历史
git log --oneline --follow kernel/sched/fair.c | head -20

# 查看某行的最后修改
git blame kernel/sched/fair.c -L 1000,1010
```

---

## 与旧书差异

| ULK3 讲的 | Bootlin 讲义 / 6.x |
|-----------|-------------------|
| 基于 2.6 源码树 | 跟随最新 LTS (6.1/6.6) |
| 手动浏览源码 | 用 Elixir 交叉索引工具 |
| 无设备树概念 | 设备树是嵌入式核心 |
| 无 BPF | eBPF 是现代观测核心 |
| 无 io_uring | io_uring 替代 AIO |
| SLAB 分配器 | SLUB + folio |

---

## HFT 关联

| 操作 | HFT 用途 |
|------|----------|
| 编译自定义内核 | 启用 PREEMPT_RT, KASAN, KFENCE |
| 源码浏览 | 理解网卡驱动发包路径 (ixgbe_xmit_frame) |
| git blame | 追踪调度器/网络栈变更原因 |
| menuconfig | 精确控制编译选项 |

> **HFT 实践：** 生产环境用 `make defconfig` + 手动关闭不需要的子系统（减少内核体积和攻击面），开发环境用 KASAN/KFENCE 发现内存 bug。

---

## 自测题

<details>
<summary>Q1: ULK3 的源码浏览方法在现代内核上有什么问题？</summary>

ULK3 基于 2.6 源码，大量函数、结构体已被重命名或删除（如 O(1) 调度器、SLAB 分配器、ticket spinlock）。直接对照 ULK3 在 6.x 源码中查找会找不到。应使用 Elixir (elixir.bootlin.com) 在线交叉索引，或本地 cscope/ctags + git blame 追踪变更历史。
</details>

<details>
<summary>Q2: 树莓派 5 编译内核时 ARCH 和 CROSS_COMPILE 参数的作用？</summary>

`ARCH=arm64` 告诉 Makefile 使用 `arch/arm64/` 目录的架构相关代码。`CROSS_COMPILE=aarch64-linux-gnu-` 指定交叉编译工具链前缀（在 x86 主机上编译 ARM64 二进制）。如果不指定 CROSS_COMPILE，会尝试用本机 gcc 编译（在 x86 上会生成 x86 代码）。
</details>

<details>
<summary>Q3: HFT 场景下，编译内核时哪些 CONFIG 选项是必须的？</summary>

必须: `CONFIG_PREEMPT_RT=y`（实时抢占）、`CONFIG_HZ_1000=y`（高精度时钟）。开发期: `CONFIG_KASAN=y`（内存消毒器）、`CONFIG_DEBUG_INFO=y`（调试符号）、`CONFIG_KFENCE=y`（轻量内存检测）。生产期可关闭 KASAN/DEBUG_INFO 以提升性能，保留 KFENCE。
</details>

---

## 交叉引用

- [01-kernel-space-vs-user-space.md](./01-kernel-space-vs-user-space.md) — 内核空间与子系统概览
- [chapter-07-arm64-boot](../chapter-07-arm64-boot/) — ARM64 启动流程详解
- [chapter-10-preempt-rt](../chapter-10-preempt-rt/) — PREEMPT_RT 编译与调优
