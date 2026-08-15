# 5.2 KASAN：地址消毒器

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

KASAN (Kernel Address SANitizer) 为每个内存字节维护**影子内存**，在每次内存访问时检查是否安全。是开发期最强大的内存错误检测工具。

## 工作原理

```
每 8 字节内存 → 1 字节影子内存
影子值:
  0x00  — 全部 8 字节可访问
  0x01-0x07 — 前 N 字节可访问
  0xFE  — redzone (越界检测)
  0xFB  — 已释放 (UAF 检测)
  0xFA  — 未分配
  0xFC  — stack redzone
  0xF9  — kmalloc redzone

内存布局:
┌─────────────────────────────────────────────┐
│  实际内存:  [obj1][redzone][obj2][redzone]  │
│  影子内存:  [00..][FE FE..][00..][FE FE..]  │
└─────────────────────────────────────────────┘
```

## 启用 KASAN

```bash
# 内核配置 (ARM64)
CONFIG_KASAN=y
CONFIG_KASAN_GENERIC=y     # 通用模式（软件实现）
# 或
CONFIG_KASAN_INLINE=y       # 内联检查 (更快但更大镜像)
# 或
CONFIG_KASAN_OUTLINE=y      # 函数调用检查 (更小但更慢)

# 编译
make ARCH=arm64 kasan.config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 树莓派 5 注意事项:
# KASAN 需要 1GB+ 内存，树莓派 5 (4GB/8GB) 可以
# 性能开销约 2-3x slowdown + ~1/8 内存用于影子
```

## KASAN 检测的错误类型

| 类型 | 影子值 | 说明 | 示例 |
|------|--------|------|------|
| `slab-out-of-bounds` | 0xFE | 越界访问 slab 对象 | buf[64] = 'x' (buf 只有 64 字节) |
| `use-after-free` | 0xFB | 访问已释放的 slab 对象 | kfree(ptr); ptr[0] = 'y' |
| `wild-memory-access` | 0xFA | 访问未分配的内存 | *(int *)0x12345678 = 42 |
| `null-ptr-deref` | - | 空指针解引用 | *NULL = 42 |
| `stack-out-of-bounds` | 0xFC | 栈缓冲区越界 | char buf[8]; buf[16] = 'x' |
| `global-out-of-bounds` | 0xF9 | 全局变量越界 | 全局数组越界访问 |

## KASAN 报告示例

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

### 报告解读

```
关键信息:
1. 错误类型: slab-out-of-bounds (越界)
2. 访问操作: Write of size 4 (写 4 字节)
3. 访问地址: ffff000012345678
4. 访问者: my_app/1234
5. 调用栈: my_driver_write → vfs_write → ksys_write
6. 分配栈: __kmalloc → my_driver_init
7. 对象信息: kmalloc-128, 128字节, 偏移 120 (越界 8 字节)
```

## KASAN quarantine（隔离区）

```
kfree(ptr) 后的流程:

普通 SLUB:
  kfree(ptr) → 对象回到 freelist → 立即可以被重新分配

KASAN 启用时:
  kfree(ptr) → 影子标记 0xFB → 进入 quarantine 队列
  → 延迟一段时间后才回到 freelist
  → 期间如果 UAF，KASAN 检测到 0xFB 影子值

quarantine 大小:
  CONFIG_KASAN_QUARANTINE_SIZE (默认 1MB)
  太小: UAF 被新分配掩盖
  太大: 内存浪费
```

## KASAN_INLINE vs KASAN_OUTLINE

| 模式 | 检查方式 | 性能 | 镜像大小 | 推荐 |
|------|---------|------|---------|------|
| INLINE | 每次访问内联 3-5 条检查指令 | 快 | 大 (~2x) | ✅ 调试 |
| OUTLINE | 每次访问调用 __asan_check 函数 | 慢 | 小 | 空间受限 |

## HFT 关联

KASAN 在 HFT 开发中的作用：

1. **开发期必开**：每次代码变更后用 KASAN 跑测试
2. **DMA buffer 检测**：KASAN 不直接检测 DMA，但检测驱动对 buffer 的越界访问
3. **UAF 检测**：quarantine 确保释放后的对象不会被立即重用
4. **生产不可用**：2-3x slowdown 破坏实时性，生产用 KFENCE 替代

```bash
# HFT 开发工作流
# 1. 编译 KASAN 内核
make ARCH=arm64 kasan.config -j$(nproc)

# 2. 运行测试
./run_hft_tests.sh

# 3. 检查 KASAN 报告
dmesg | grep -A 30 "BUG: KASAN"

# 4. 修复所有 KASAN 报告后切换到普通内核
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KASAN 的影子内存开销是多少？

> 每 8 字节内存需要 1 字节影子内存，开销为 1/8 (12.5%)。对于 4GB 内存的系统，影子内存约 512MB。此外还有性能开销：KASAN_INLINE 模式下每次内存访问额外 3-5 条指令检查影子内存，整体 slowdown 约 2-3 倍。

**Q2:** KASAN 如何检测 Use-After-Free？

> 当 slab 对象被 kfree 释放时，KASAN 将其影子内存标记为 0xFB (freed)，并将对象放入 quarantine 隔离区延迟回收。如果后续代码访问该地址，KASAN 检查影子值发现是 0xFB，报告 use-after-free。quarantine 确保 UAF 不会被新分配掩盖。

**Q3:** KASAN 的 quarantine（隔离区）是什么？为什么需要？

> quarantine 延迟 slab 对象的实际回收。释放后对象标记为 0xFB 但不立即放回 slab freelist，先进入 quarantine 队列。这样如果 UAF 发生，KASAN 能检测到。没有 quarantine，释放的对象可能被立即重新分配，UAF 访问到新对象不会报错。

**Q4:** KASAN_INLINE 和 KASAN_OUTLINE 的区别？HFT 调试应该选哪个？

> INLINE：检查代码内联到每次内存访问，快但镜像大。OUTLINE：检查代码是函数调用，小但慢。HFT 调试选 INLINE（更快，减少调试时的时序偏差）。生产环境不用 KASAN（开销太大）。

**Q5:** KASAN 报告中 "Allocated by task" 有什么作用？

> 显示分配这个内存块的调用栈。配合 "BUG in" 的访问栈，可以对比分配和访问的上下文，判断是哪个分配出了问题。例如 UAF 中，分配栈显示在哪里分配的，访问栈显示在哪里释放后访问的，两者配合定位 bug。

</details>

## 交叉引用

- [05.6 ch05 内存错误类型](../../chapter-05-memory-debug-1/notes/01-memory-error-types.md)
- [05.6 ch05 SLUB debug](../../chapter-05-memory-debug-1/notes/04-slub-debug.md)
- [05.6 ch06 KFENCE](../../chapter-06-memory-debug-2/notes/01-kfence.md)
