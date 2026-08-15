# KGDB 与 QEMU 虚拟机调试

> 🔴 精读

## 概念详解

### QEMU + KGDB 调试

```bash
# 1. 启动 QEMU (带 GDB 支持)
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a76 \
    -kernel Image \
    -append "console=ttyAMA0 kgdboc=ttyAMA0,115200 nokaslr" \
    -serial mon:stdio \
    -gdb tcp::5555 \
    -S \
    -nographic

# -S: 启动时暂停 (等待 GDB 连接)
# -gdb tcp::5555: GDB 端口 5555
```

### QEMU KGDB 优势

| 优势 | 说明 |
|------|------|
| 无需物理串口 | 通过 TCP 连接 |
| 可调试启动早期 | 从第一条指令开始 |
| 可重复 | 快照 + 回放 |
| 无需硬件 | 开发机上完成 |
| 速度快 | TCP 比 UART 快 |

### QEMU GDB vs KGDB

QEMU 自带 GDB stub（`-gdb` 参数），不需要内核 KGDB 支持。两者可以配合使用：

| 方式 | 配置 | 优势 |
|------|------|------|
| QEMU GDB | `-gdb tcp::5555 -S` | 从第一条指令调试 |
| KGDB | `kgdboc=...` | 调试运行中的内核 |
| 两者配合 | 先 QEMU GDB 启动，后 KGDB | 全流程调试 |

### 调试启动流程

```gdb
# 从第一条内核指令开始调试
(gdb) target remote :5555
(gdb) break __primary_entry    # ARM64 内核入口
(gdb) continue
(gdb) next                     # 逐步执行启动代码
(gdb) break start_kernel       # C 代码入口
(gdb) continue
```

### 调试 HFT 模块

```bash
# 1. QEMU 启动内核
qemu-system-aarch64 -M virt -kernel Image \
    -append "console=ttyAMA0 nokaslr" \
    -gdb tcp::5555 -nographic

# 2. 在 QEMU 中加载模块
# (通过串口终端)
insmod my_hft_module.ko

# 3. GDB 连接
aarch64-linux-gnu-gdb vmlinux
(gdb) target remote :5555
(gdb) # QEMU GDB 自动暂停内核

# 4. 加载模块符号
(gdb) add-symbol-file my_hft_module.ko 0xffff...

# 5. 设断点并调试
(gdb) break on_trade_signal
(gdb) continue
```

### QEMU 快照与回放

```bash
# 保存 VM 快照
(qemu) savevm snapshot1

# 恢复快照
(qemu) loadvm snapshot1

# 用途: 复现并发 bug
# 1. 运行到 bug 出现
# 2. 保存快照
# 3. 恢复快照
# 4. 用 GDB 逐步分析
```

### HFT 关联应用

QEMU + KGDB 适合开发期调试内核模块——不需要每次部署到树莓派，快速迭代。

```bash
# HFT 开发流程
# 1. 在 PC 上用 QEMU 启动内核
# 2. 通过 TCP 连接 GDB
# 3. 加载 HFT 模块
# 4. 设断点、单步、查看变量
# 5. 修改代码后重新编译模块
# 6. 重新加载模块（不需要重启 QEMU）

# 优势: 编译快（在 PC 上）、调试快（TCP 连接）、可复现（快照）
```

### QEMU vs 树莓派调试

| 方面 | QEMU | 树莓派 |
|------|------|--------|
| 连接方式 | TCP (快) | UART (慢) |
| 硬件精度 | 模拟 | 真实 |
| 启动调试 | 从第一条指令 | 需要特殊配置 |
| 快照回放 | 支持 | 不支持 |
| 外设 | 模拟 | 真实 |
| 适用 | 开发期迭代 | 最终验证 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** QEMU KGDB 调试相比物理树莓派有什么优势？

> QEMU 通过 TCP 连接 GDB（无需物理串口线），可以从第一条内核指令开始调试，支持快照回放，且在开发机上完成。缺点是性能不真实，无法测试真实硬件 I/O 延迟。

**Q2:** QEMU + KGDB 相比物理机 KGDB 有什么优势？

> (1) 快照/回放：QEMU 可以保存和恢复 VM 状态；(2) GDB stub 内置：QEMU 自带 GDB remote stub，不需要 KGDB 内核支持；(3) 无需串口硬件；(4) 可以模拟多种架构。劣势：无法测试真实硬件行为。

**Q3:** QEMU 的 `-S` 参数有什么作用？

> `-S` 让 QEMU 启动时暂停 CPU，等待 GDB 连接后再开始执行。这样可以从第一条内核指令开始调试，适合调试启动流程（如 head.S 汇编代码）。

**Q4:** QEMU 快照如何帮助调试并发 bug？

> 并发 bug 通常难以复现。用 QEMU 快照：(1) 运行到 bug 出现；(2) 保存快照；(3) 恢复快照；(4) 用 GDB 逐步分析。可以反复在同一状态下调试，不需要等待 bug 再次随机触发。

**Q5:** HFT 模块开发为什么推荐 QEMU 而非每次部署到树莓派？

> (1) 编译快（PC 比树莓派快得多）；(2) 部署快（不需要拷贝到 SD 卡）；(3) 调试快（TCP 比 UART 快）；(4) 可复现（快照）。开发期用 QEMU 迭代，最终验证在树莓派上做。

</details>

## 交叉引用

- [05.6 ch11 KGDB 原理与架构](../../chapter-11-kgdb/notes/01-kgdb-architecture.md)
- [05.6 ch11 串口配置](../../chapter-11-kgdb/notes/02-uart-setup.md)
- [05.6 ch11 调试内核模块](../../chapter-11-kgdb/notes/05-module-debugging.md)
