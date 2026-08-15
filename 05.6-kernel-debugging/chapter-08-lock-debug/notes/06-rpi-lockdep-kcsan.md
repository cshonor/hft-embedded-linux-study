# 在树莓派上启用 LOCKDEP / KCSAN

> 🔴 精读

## 概念详解

### 树莓派 5 内核配置

```bash
# 获取树莓派内核源码
git clone https://github.com/raspberrypi/linux.git
cd linux
git checkout rpi-6.1.y

# 基础配置
make ARCH=arm64 bcm2712_defconfig

# 启用调试选项
./scripts/config --enable DEBUG_INFO
./scripts/config --enable DEBUG_INFO_DWARF5
./scripts/config --enable LOCKDEP
./scripts/config --enable LOCK_STAT
./scripts/config --enable KASAN
./scripts/config --enable KCSAN
./scripts/config --enable SLUB_DEBUG
./scripts/config --enable DEBUG_LIST
./scripts/config --enable DEBUG_SG
./scripts/config --enable PROVE_LOCKING
./scripts/config --enable DEBUG_ATOMIC_SLEEP

make ARCH=arm64 olddefconfig

# 编译
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
```

### 验证调试选项

```bash
zcat /proc/config.gz | grep -E "LOCKDEP|KASAN|KCSAN|SLUB_DEBUG"
# CONFIG_LOCKDEP=y
# CONFIG_KASAN=y
# CONFIG_KCSAN=y
# CONFIG_SLUB_DEBUG=y

cat /proc/lock_stat  # 如果存在则 LOCKDEP 启用
dmesg | grep -i kasan  # KernelAddressSanitizer initialized
dmesg | grep -i kcsan  # KCSAN: initialized
```

### 各工具的内存和性能开销

| 工具 | 内存开销 | 性能开销 | 适用场景 |
|------|---------|---------|---------|
| LOCKDEP | ~10-50MB | ~2-5x slowdown | 锁序检测 |
| lock_stat | 同 LOCKDEP | ~3-8x slowdown | 锁竞争分析 |
| KASAN | ~1/8 物理内存 | ~2-3x slowdown | 内存错误检测 |
| KCSAN | ~10-20MB | ~1.5-2x slowdown | 数据竞争检测 |
| 全部启用 | ~512MB+ | ~5-10x slowdown | 完整调试 |

### 树莓派 5 资源评估

```bash
# 树莓派 5 (4GB)
物理内存: 4GB
  - KASAN 影子内存: ~512MB (1/8)
  - 内核镜像 + 模块: ~200-300MB (带 DEBUG_INFO)
  - 用户空间可用: ~3.2GB
  → 足够运行调试内核 + 测试程序

# 树莓派 5 (8GB) 更宽裕
```

### 调试选项组合推荐

| 场景 | 启用选项 | 说明 |
|------|---------|------|
| 开发阶段 | LOCKDEP + KASAN + KCSAN | 全面检测 |
| 锁调试 | LOCKDEP + lock_stat + PROVE_LOCKING | 专注锁问题 |
| 内存调试 | KASAN + SLUB_DEBUG + kmemleak | 专注内存问题 |
| HFT staging | LOCKDEP + KCSAN | 检测并发问题 |
| 生产环境 | 无调试选项 | 性能优先 |

### Boot 参数调试

```bash
# /boot/cmdline.txt
console=serial0,115200 console=tty1 lockdep nokaslr panic_on_oops=1 panic=5
```

### 注意事项

| 问题 | 说明 | 解决方案 |
|------|------|---------|
| 内存需求 | KASAN 需要 ~512MB 影子内存 | 树莓派 5 (4GB+) 可以 |
| 性能影响 | 全部启用 slowdown ~5-10x | 仅开发/staging 使用 |
| 编译时间 | DEBUG_INFO 增加编译时间 | 首次约 30-60 分钟 |
| 镜像大小 | DEBUG_INFO 使 Image 增大约 10 倍 | 保留 vmlinux 在编译机 |

### HFT 关联应用

```bash
# HFT staging 环境的树莓派调试配置
# /boot/cmdline.txt
console=serial0,115200 lockdep nokaslr panic_on_oops=1 panic=5

# /etc/sysctl.d/99-hft-debug.conf
kernel.lock_stat = 1
kernel.panic_on_oops = 1
kernel.panic = 5
```

### 树莓派 vs QEMU 调试

| 方面 | 树莓派 5 | QEMU |
|------|---------|------|
| 硬件精度 | 真实硬件 | 模拟 |
| 性能 | 真实（慢但可信） | 快但可能不精确 |
| GDB 调试 | KGDB over UART | QEMU 内置 GDB |
| 推荐 | 最终验证 | 快速迭代 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KASAN 和 LOCKDEP 能同时启用吗？有什么影响？

> 可以同时启用，且推荐在开发阶段同时启用。检测维度不同，同时启用的 slowdown 是叠加的（~5-10x），不适合生产环境。

**Q2:** 树莓派 5 启用 KASAN 需要注意什么？

> KASAN 影子内存占用 1/8 的物理内存。树莓派 5 (4GB) 需要约 512MB 影子内存。建议使用 8GB+ SD 卡。

**Q3:** 在树莓派上做内核锁调试，硬件有什么限制？

> (1) 4 核够用但性能有限；(2) 没有 JTAG，依赖 KGDB over UART；(3) SD 卡 I/O 慢，建议用 USB SSD。

**Q4:** 树莓派调试内核为什么建议禁用 KASLR？

> KASLR 使内核加载到随机地址，分析 Oops 时需要计算偏移。调试时禁用 KASLR (`nokaslr`) 简化地址解析。生产环境应启用 KASLR。

**Q5:** 树莓派和 QEMU 在内核调试中各自的优劣势？

> 树莓派：真实硬件，外设精确，适合最终验证；但编译慢、部署繁琐。QEMU：快速迭代、内置 GDB；但硬件模拟可能不精确。推荐：QEMU 快速迭代 → 树莓派最终验证。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP 锁依赖检测器](../../chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 KCSAN 数据竞争检测器](../../chapter-08-lock-debug/notes/05-kcsan.md)
- [05.6 ch05 KASAN 内存检测](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch11 KGDB UART 调试](../../chapter-11-kgdb/notes/01-kgdb-architecture.md)
