# 5.2 KASAN：地址消毒器

> 🔴 精读

## 本节要点

### KASAN (Kernel Address SANitizer)

KASAN 为每个内存字节维护**影子内存 (shadow memory)**，记录该字节是否可安全访问。每次内存访问编译时自动插入检查代码。

### 工作原理

```
每 8 字节内存 → 1 字节影子内存
影子值:
  0x00  — 全部 8 字节可访问
  0x01-0x07 — 前 N 字节可访问
  0xFE  — redzone (越界检测)
  0xFB  — 已释放 (UAF 检测)
  0xFA  — 未分配
```

### 启用 KASAN

```bash
# 内核配置 (ARM64)
CONFIG_KASAN=y
CONFIG_KASAN_INLINE=y        # 内联检查 (更快但更大镜像)
# 或
CONFIG_KASAN_OUTLINE=y       # 函数调用检查 (更小但更慢)

# 编译
make ARCH=arm64 kasan.config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 树莓派 5 注意事项:
# KASAN 需要 1GB+ 内存，树莓派 5 (4GB/8GB) 可以
# 性能开销约 2-3x slowdown + ~1/8 内存用于影子
```

### KASAN 报告示例

```
[   12.345678] ==================================================================
[   12.345680] BUG: KASAN: slab-out-of-bounds in my_driver_write+0x3c/0x100
[   12.345682] Write of size 4 at addr ffff000012345678 by task my_app/1234

[   12.345690] CPU: 2 PID: 1234 Comm: my_app Not tainted 6.1.63
[   12.345695] Call trace:
[   12.345700]  my_driver_write+0x3c/0x100
[   12.345705]  vfs_write+0xf4/0x2b0
[   12.345710]  ksys_write+0x74/0x100

[   12.345720] Allocated by task 1234:
[   12.345725]  kasan_save_stack+0x28/0x50
[   12.345730]  __kmalloc+0xc0/0x1d0
[   12.345735]  my_driver_init+0x48/0x200

[   12.345740] The buggy address belongs to the object at ffff000012345600
[   12.345745]  which belongs to the cache kmalloc-128 of size 128
[   12.345750] The buggy address is located 120 bytes inside of
[   12.345755]  128-byte region [ffff000012345600, ffff000012345680)
```

### KASAN 检测的错误类型

| 类型 | 影子值 | 说明 |
|------|--------|------|
| `slab-out-of-bounds` | 0xFE | 越界访问 slab 对象 |
| `use-after-free` | 0xFB | 访问已释放的 slab 对象 |
| `wild-memory-access` | 0xFA | 访问未分配的内存 |
| `null-ptr-deref` | - | 空指针解引用 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KASAN 的影子内存开销是多少？

> 每 8 字节内存需要 1 字节影子内存，开销为 1/8 (12.5%)。对于 4GB 内存的系统，影子内存约 512MB。此外还有性能开销：KASAN_INLINE 模式下每次内存访问额外 3-5 条指令检查影子内存，整体 slowdown 约 2-3 倍。

**Q2:** KASAN 如何检测 Use-After-Free？

> 当 slab 对象被 kfree 释放时，KASAN 将其影子内存标记为 0xFB (freed)。如果后续代码访问该地址，KASAN 检查影子值发现是 0xFB，报告 use-after-free。KASAN 还会延迟 slab 对象的实际回收（quarantine 隔离区），确保 UAF 不会被新分配掩盖。


**Q:** KASAN 的 quarantine（隔离区）是什么？为什么需要？

> quarantine 延迟 slab 对象的实际回收。释放后对象标记为 0xFB 但不立即放回 slab freelist，先进入 quarantine 队列。这样如果 UAF 发生，KASAN 能检测到。没有 quarantine，释放的对象可能被立即重新分配，UAF 访问到新对象不会报错。

**Q:** KASAN_INLINE 和 KASAN_OUTLINE 的区别？HFT 调试应该选哪个？

> INLINE：检查代码内联到每次内存访问，快但镜像大。OUTLINE：检查代码是函数调用，小但慢。HFT 调试选 INLINE（更快，减少调试时的时序偏差）。生产环境不用 KASAN（开销太大）。

</details>

## 交叉引用

- [05.6 ch05 SLUB debug](chapter-05-memory-debug-1/notes/section-5-4.md)
