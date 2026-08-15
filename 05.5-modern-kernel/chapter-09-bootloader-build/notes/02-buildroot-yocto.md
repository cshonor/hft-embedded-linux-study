# Bootlin: Buildroot / Yocto — 嵌入式 Linux 构建

> **来源:** [Bootlin Embedded Linux Training](https://bootlin.com/docs/)
> **主题:** 嵌入式 Linux 系统构建工具
> **对标旧书:** 无覆盖

---

## 讲义要点

### Buildroot vs Yocto 对比

| 特性 | Buildroot | Yocto Project |
|------|-----------|---------------|
| **复杂度** | 低 (Makefile 驱动) | 高 (BitBake + layers) |
| **构建速度** | 快 (单次) | 慢 (多阶段) |
| **包管理** | 无 (重新编译升级) | 有 (RPM/DEB/IPK) |
| **适用场景** | 小型嵌入式 (路由器/IoT) | 大型嵌入式 (手机/车机) |
| **学习曲线** | 低 | 陡峭 |
| **社区** | 活跃 | 非常活跃 (大厂支持) |

### Buildroot 基本流程

```bash
# 1. 获取 Buildroot
git clone https://github.com/buildroot/buildroot.git
cd buildroot

# 2. 配置 (树莓派 5)
make raspberrypi5_64_defconfig

# 3. 自定义配置
make menuconfig
#   - Target architecture: AArch64
#   - Toolchain: External (或 Internal)
#   - System: hostname, root password
#   - Packages: 选需要的用户空间包
#   - Kernel: 自定义内核版本/配置
#   - Bootloaders: U-Boot

# 4. 编译 (首次约 30-60 分钟)
make -j$(nproc)

# 5. 输出
ls output/images/
# sdcard.img      — 完整 SD 卡镜像
# zImage          — 内核镜像
# bcm2712-rpi-5-b.dtb — 设备树
# rootfs.tar.gz   — 根文件系统
# u-boot.bin      — U-Boot

# 6. 烧录到 SD 卡
dd if=output/images/sdcard.img of=/dev/sdX bs=4M
```

### Buildroot 自定义

```makefile
# output/customize.sh 或 package/ 目录结构
# 添加自定义包
mkdir -p package/myapp
cat > package/myapp/Config.in << 'EOF'
config BR2_PACKAGE_MYAPP
    bool "myapp"
    help
      My custom application
EOF

cat > package/myapp/myapp.mk << 'EOF'
MYAPP_VERSION = 1.0
MYAPP_SITE = $(BR2_EXTERNAL_MYAPP_PATH)/src/myapp
MYAPP_SITE_METHOD = local
MYAPP_DEPENDENCIES = libev

define MYAPP_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" -C $(@D)
endef

define MYAPP_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/myapp $(TARGET_DIR)/usr/bin/myapp
endef

$(eval $(generic-package))
EOF
```

### Yocto 基本流程

```bash
# 1. 获取 Yocto
git clone git://git.yoctoproject.org/poky
cd poky
git checkout kirkstone  # LTS 版本

# 2. 初始化环境
source oe-init-build-env

# 3. 配置 (conf/local.conf)
# MACHINE ??= "raspberrypi4-64"
# 或添加 meta-raspberrypi layer

# 4. 编译
bitbake core-image-minimal

# 5. 输出
ls tmp/deploy/images/raspberrypi4-64/
# core-image-minimal-raspberrypi4-64.wic.bz2 — SD 卡镜像
```

### Yocto Layer / Recipe 概念

| 概念 | 说明 |
|------|------|
| **Layer** | 可组合的配置/配方集合 (meta-raspberrypi, meta-openembedded) |
| **Recipe** (.bb) | 一个包的构建配方 (源码位置、依赖、编译/安装步骤) |
| **bbappend** | 对现有 recipe 的增量修改 |
| **Image** | 一组 recipe 组成的完整镜像 |

---

## 动手实验

```bash
# Buildroot 实验 (推荐树莓派 5)
# 1. 构建最小系统
make raspberrypi5_64_defconfig
make -j$(nproc)

# 2. 修改配置添加调试工具
make menuconfig
# → Target packages → Debugging, profiling and benchmark
#   [*] strace
#   [*] perf
# → Target packages → Networking
#   [*] tcpdump
# → Target packages → Interpreter
#   [*] python3

# 3. 重新编译
make -j$(nproc)

# 4. 烧录测试
dd if=output/images/sdcard.img of=/dev/sdX bs=4M
# 插入树莓派 5 启动

# 5. 添加自定义内核 patch
make menuconfig
# → Kernel → Custom kernel patches
# 指定 patch 文件路径

# 6. 单独重新编译内核
make linux-rebuild
# 重新生成镜像
make -j$(nproc)
```

---

## 与旧书差异

| 旧书 | Bootlin 讲义 |
|------|-------------|
| 不覆盖系统构建 | Buildroot/Yocto 是嵌入式标配 |
| 手动交叉编译 | 自动化构建系统 |
| 手动制作 rootfs | 自动生成 rootfs + 镜像 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Buildroot 和 Yocto 的主要区别是什么？什么时候选哪个？

> Buildroot 简单快速，用 Makefile 驱动，无包管理，适合小型设备（IoT、路由器）。Yocto 复杂强大，用 BitBake + Layer 架构，支持包管理（RPM/DEB），适合大型设备（车机、手机）。简单项目选 Buildroot，需要长期维护和 OTA 升级选 Yocto。

**Q2:** Buildroot 升级一个包为什么需要重新编译整个系统？

> Buildroot 不生成包管理器（默认），所有包直接编译进 rootfs。升级一个包需要重新生成 rootfs 镜像。Yocto 可以生成独立包（.rpm/.deb），升级时只需替换该包。这是两者的核心架构差异。

**Q3:** Yocto 的 Layer 架构有什么好处？

> Layer 可组合——不同团队/厂商维护各自的 layer（BSP layer、UI layer、app layer），通过叠加组合成最终镜像。升级 BSP 时只需更新 BSP layer，不影响 app layer。这比 Buildroot 的单一配置更适合大型项目协作。

</details>
