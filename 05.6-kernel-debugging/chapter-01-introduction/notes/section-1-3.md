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


**Q:** 内核调试需要哪些硬件工具？虚拟机和物理机调试各有什么优劣？

> 硬件：串口线（KGDB/earlycon）、JTAG（硬件断点）、逻辑分析仪（时序问题）。VM 优势：快照、易重置、GDB stub 内置。物理机优势：真实硬件行为（DMA/中断/时序），但调试设施少。HFT 开发推荐 QEMU 做逻辑验证 + 物理机做性能验证。

**Q:** 为什么调试内核需要编译 CONFIG_DEBUG_INFO=y？

> DEBUG_INFO 生成 DWARF 调试信息，addr2line 需要 it 将地址映射到源码行。GDB 连接 KGDB 也需要 vmlinux 带 DEBUG_INFO 才能设置源码级断点。代价：vmlinux 文件变大（~500MB），但运行时无性能影响。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/section-7-4.md)
- [05.6 ch11 KGDB](chapter-11-kgdb/notes/section-11-3.md)
