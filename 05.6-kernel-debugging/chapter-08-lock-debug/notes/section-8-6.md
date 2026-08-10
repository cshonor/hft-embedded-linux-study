# 8.6 在树莓派上启用 LOCKDEP / KCSAN

> 🔴 精读

## 本节要点

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

make ARCH=arm64 olddefconfig

# 编译 (KASAN 需要更多内存和时间)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
```

### 验证调试选项

```bash
# 在树莓派上启动后验证
zcat /proc/config.gz | grep -E "LOCKDEP|KASAN|KCSAN|SLUB_DEBUG"
# CONFIG_LOCKDEP=y
# CONFIG_KASAN=y
# CONFIG_KCSAN=y
# CONFIG_SLUB_DEBUG=y

# 检查 LOCKDEP 是否激活
cat /proc/lock_stat  # 如果存在则 LOCKDEP 启用

# 检查 KASAN
dmesg | grep -i kasan
# [    0.000000] KernelAddressSanitizer initialized
```

### 注意事项

| 问题 | 说明 |
|------|------|
| 内存需求 | KASAN 需要 ~512MB 影子内存，树莓派 5 (4GB+) 可以 |
| 性能影响 | KASAN+LOCKDEP+KCSAN 同时启用，slowdown ~5-10x |
| 编译时间 | DEBUG_INFO 增加编译时间，首次约 30-60 分钟 |
| 内核镜像大小 | DEBUG_INFO 使 Image 增大约 10 倍（保留 vmlinux） |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KASAN 和 LOCKDEP 能同时启用吗？有什么影响？

> 可以同时启用，且推荐在开发阶段同时启用。KASAN 检测内存错误，LOCKDEP 检测锁问题，两者检测维度不同。同时启用的 slowdown 是叠加的（~5-10x），不适合生产环境，但开发阶段能一次性发现多类问题。

**Q2:** 树莓派 5 启用 KASAN 需要注意什么？

> KASAN 影子内存占用 1/8 的物理内存。树莓派 5 (4GB) 需要约 512MB 影子内存，剩余可用内存约 3.5GB，足够运行。但如果同时启用 DEBUG_INFO，内核镜像会很大（数百 MB），需要足够的 SD 卡空间。建议使用 8GB 或更大的 SD 卡。


**Q:** 在树莓派上做内核锁调试，硬件有什么限制？

> (1) 树莓派 4 核够用但性能有限，LOCKDEP + KCSAN 同时开会很慢；(2) 树莓派 5 (4GB/8GB) 可以跑 KASAN + LOCKDEP；(3) 没有 JTAG，依赖 KGDB over UART；(4) SD 卡 I/O 慢，建议用 USB SSD 减少 I/O 等待。

</details>

## 交叉引用

- [05.6 ch11 KGDB UART](chapter-11-kgdb/notes/section-11-2.md)
