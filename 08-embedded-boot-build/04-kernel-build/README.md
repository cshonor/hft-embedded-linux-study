# 04 · 内核构建：从 defconfig 到能启动的 Image

> **本节讲什么：** 拉内核树、选配置、交叉编译、装模块与 DTB，最终替换板子上的运行内核。
> **对应动手：** [P5 · B1](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（`bcm2712_defconfig` 级配置 / 裁剪 / 交叉编译 / 替换运行内核）

---

## 要点

| 概念 | 说明 |
|------|------|
| **defconfig** | 厂商给的基线配置。**永远从它开始**，不要从零 `menuconfig` |
| **裁剪的意义** | 少一个驱动 = 少一份代码进内核 = 少一次 probe = 启动更快、攻击面更小 |
| **`=y` vs `=m`** | 编进内核 vs 编成可加载模块。板级必要驱动（如 MMC、串口）必须 `=y`，否则 initrd 都救不了 |
| **DTB 要一起编** | 内核和 DTB 必须同源同版本，否则 `compatible` 对不上 |
| **modules_install** | 模块要装到 rootfs 的 `/lib/modules/$(uname -r)`，版本号必须完全一致 |

---

## 关键命令（Pi 5 · aarch64）

```bash
# 拉源码
git clone --depth=1 --branch rpi-6.12.y https://github.com/raspberrypi/linux
cd linux

# 配置
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2712_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig   # 按需裁剪

# 编译（镜像 + 模块 + DTB）
make -j$(nproc) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image.gz modules dtbs

# 安装模块到 rootfs 挂载点
sudo make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
     INSTALL_MOD_PATH=/mnt/rootfs modules_install
```

**校验产物：**

```bash
file arch/arm64/boot/Image.gz                 # gzip compressed
ls arch/arm64/boot/dts/broadcom/*.dtb | head
```

**回滚预案：** 永远保留原内核副本（`config.txt` 里用 `kernel=` 指向新内核，改名即可回退）。**刷坏 bootloader 的代价远大于刷坏内核。**

---

## HFT / 嵌入式关联

- **裁剪 = 延迟确定性**：无关驱动越少，中断处理路径上的不确定性越少。
- **PREEMPT_RT**：低延迟场景要开 `CONFIG_PREEMPT_RT`，但它会牺牲一部分吞吐——这是 HFT 里"延迟 vs 吞吐"经典权衡在内核配置层的体现（深入见 [05-linux-kernel](../../05-linux-kernel/) / [05.5](../../05.5-modern-kernel/)）。
- 内核版本号必须和模块路径严格对应，这个"版本即契约"的思路在 [02-toolchain](../02-toolchain/) 的 sysroot 那里已经出现过一次。

---

## 验收

- [ ] 自编内核能在 Pi 5 上启动，`uname -r` 显示自己的版本号
- [ ] 能解释 `=y` 与 `=m` 的取舍，说出至少一个必须 `=y` 的驱动
- [ ] 编过 DTB，且知道它与内核版本的绑定关系
- [ ] 有可回滚方案，并且**真的验证过回滚**

---

## 衔接

- **上一步：** [03-u-boot](../03-u-boot/)
- **下一步：** [05-rootfs](../05-rootfs/)
- **卡住查书：** MELP Ch4 · Primer Ch4–5
