# 1.3 开发环境搭建

> ⬜ 跳读

## 本节要点

| 方式 | 优势 | 劣势 |
|------|------|------|
| QEMU 虚拟机 | 快速迭代、无需硬件 | 性能不真实、无真实设备 |
| 原生 Linux (树莓派 5) | 真实硬件、真实延迟 | 编译部署慢、崩溃需重启 |

## 树莓派 5 开发环境

```bash
# 交叉编译内核
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2712_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules

# 部署到 SD 卡
cp arch/arm64/boot/Image /boot/firmware/kernel8.img
cp arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dtb /boot/firmware/
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_install INSTALL_MOD_PATH=/mnt/sdcard

# 启用调试选项
echo "CONFIG_DEBUG_INFO=y" >> .config
echo "CONFIG_GDB_SCRIPTS=y" >> .config
echo "CONFIG_LOCKDEP=y" >> .config
make olddefconfig
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 调试内核为什么需要重新编译？不能直接调试发行版内核吗？

> 发行版内核通常关闭了调试选项（CONFIG_DEBUG_INFO, CONFIG_LOCKDEP, CONFIG_KASAN 等），因为调试选项会增加开销。调试需要重新编译启用这些选项。可以用 CONFIG_DEBUG_INFO_NONE=n + CONFIG_DEBUG_INFO_DWARF5=y 获取调试符号。

</details>
