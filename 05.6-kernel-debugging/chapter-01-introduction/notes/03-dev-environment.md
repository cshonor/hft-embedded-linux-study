# 1.3 开发环境搭建

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

内核调试需要专门的开发环境——编译带调试符号的内核、配置串口/KGDB、准备测试硬件。

## 开发环境对比

| 方式 | 优势 | 劣势 | 适用场景 |
|------|------|------|---------|
| QEMU 虚拟机 | 快速迭代、快照回滚、无需硬件 | 性能不真实、无真实设备 | 逻辑验证、功能性调试 |
| 原生 Linux (树莓派 5) | 真实硬件、真实中断/DMA | 编译部署慢、崩溃需重启 | 驱动开发、性能调试 |
| 物理服务器 | 真实多核、真实 NUMA | 成本高、不灵活 | HFT 生产模拟 |

## 树莓派 5 开发环境

### 交叉编译内核

```bash
# 1. 获取内核源码
git clone --depth=1 https://github.com/raspberrypi/linux
cd linux

# 2. 配置
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2712_defconfig

# 3. 启用调试选项
scripts/config --enable DEBUG_INFO
scripts/config --enable DEBUG_INFO_DWARF5
scripts/config --enable GDB_SCRIPTS
scripts/config --enable LOCKDEP
scripts/config --enable KASAN  # 可选，内存调试
scripts/config --enable FTRACE
scripts/config --enable FUNCTION_TRACER
scripts/config --enable FUNCTION_GRAPH_TRACER
make olddefconfig

# 4. 编译
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules

# 5. 部署到 SD 卡
cp arch/arm64/boot/Image /boot/firmware/kernel8.img
cp arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dtb /boot/firmware/
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_install INSTALL_MOD_PATH=/mnt/sdcard
```

### QEMU 虚拟机环境

```bash
# 快速启动 QEMU（x86_64）
qemu-system-x86_64 \
    -kernel arch/x86/boot/bzImage \
    -drive file=rootfs.ext2,format=raw,if=virtio \
    -append "console=ttyS0 root=/dev/vda nokaslr" \
    -gdb tcp::1234 \      # GDB stub
    -S \                    # 启动时暂停
    -nographic

# GDB 连接
gdb vmlinux
(gdb) target remote :1234
(gdb) break start_kernel
(gdb) continue
```

### 关键调试配置项

| 配置项 | 作用 | 开销 | 推荐场景 |
|--------|------|------|---------|
| `CONFIG_DEBUG_INFO` | DWARF 调试符号 | vmlinux 变大 | 必须开启 |
| `CONFIG_GDB_SCRIPTS` | GDB Python 脚本 | 无 | 必须开启 |
| `CONFIG_LOCKDEP` | 锁依赖检测 | ~5% | 开发/测试 |
| `CONFIG_KASAN` | 地址消毒器 | 2-3x | 开发 |
| `CONFIG_KCSAN` | 数据竞争检测 | ~10% | 开发 |
| `CONFIG_FTRACE` | 函数追踪 | 无（未启用时） | 必须开启 |
| `CONFIG_KGDB` | 内核调试器 | 无 | 开发 |
| `CONFIG_KALLSYMS` | 符号表 | 极低 | 必须开启 |
| `CONFIG_KALLSYMS_ALL` | 完整符号 | ~200KB | 推荐开启 |

## 串口配置（KGDB 必需）

```bash
# 树莓派 5: GPIO 14/15 为 UART0
# /boot/firmware/config.txt 中添加:
enable_uart=1

# /boot/firmware/cmdline.txt 中添加:
console=serial0,115200 kgdboc=serial0,115200

# 连接串口（USB-TTL 转换器）
# TXD -> GPIO 15 (RXD)
# RXD -> GPIO 14 (TXD)
# GND -> GND
stty -F /dev/ttyUSB0 115200
screen /dev/ttyUSB0 115200
```

## HFT 关联

HFT 开发推荐双环境策略：
1. **QEMU**：逻辑验证——快速编译、快照回滚、GDB 断点调试
2. **物理机**（树莓派/服务器）：性能和硬件验证——真实中断延迟、DMA 行为、网卡驱动

生产环境额外准备：
- kdump 配置好，崩溃自动转储
- serial console 连接，远程可访问
- ftrace/trace-cmd 预装

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 调试内核为什么需要重新编译？不能直接调试发行版内核吗？

> 发行版内核通常关闭了调试选项（CONFIG_DEBUG_INFO, CONFIG_LOCKDEP, CONFIG_KASAN 等），因为调试选项会增加开销。调试需要重新编译启用这些选项。可以用 CONFIG_DEBUG_INFO_NONE=n + CONFIG_DEBUG_INFO_DWARF5=y 获取调试符号。

**Q2:** 内核调试需要哪些硬件工具？虚拟机和物理机调试各有什么优劣？

> 硬件：串口线（KGDB/earlycon）、JTAG（硬件断点）、逻辑分析仪（时序问题）。VM 优势：快照、易重置、GDB stub 内置。物理机优势：真实硬件行为（DMA/中断/时序），但调试设施少。HFT 开发推荐 QEMU 做逻辑验证 + 物理机做性能验证。

**Q3:** 为什么调试内核需要编译 CONFIG_DEBUG_INFO=y？

> DEBUG_INFO 生成 DWARF 调试信息，addr2line 需要 it 将地址映射到源码行。GDB 连接 KGDB 也需要 vmlinux 带 DEBUG_INFO 才能设置源码级断点。代价：vmlinux 文件变大（~500MB），但运行时无性能影响。

**Q4:** QEMU 调试和 KGDB 调试有什么区别？

> QEMU 自带 GDB stub，不需要修改内核代码，直接 `-gdb tcp::1234 -S` 即可。KGDB 需要内核编译 CONFIG_KGDB=y 并配置串口，但可以在真实硬件上使用。QEMU 更方便（快照、无硬件），KGDB 更真实（物理设备、真实中断）。

**Q5:** scripts/config 和手动编辑 .config 有什么区别？

> scripts/config 是内核提供的配置工具，可以安全地启用/禁用配置项并自动处理依赖关系。手动编辑 .config 可能遗漏依赖项，需要 `make olddefconfig` 来解决。推荐用 scripts/config + olddefconfig 的组合。

</details>

## 交叉引用

- [05.6 ch07 addr2line](chapter-07-oops/notes/04-addr2line.md)
- [05.6 ch11 KGDB](chapter-11-kgdb/notes/01-kgdb-architecture.md)
