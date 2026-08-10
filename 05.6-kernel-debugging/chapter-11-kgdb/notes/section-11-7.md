# 11.7 KGDB 与 QEMU 虚拟机调试

> 🔴 精读

## 本节要点

### QEMU + KGDB 调试

```bash
# 1. 启动 QEMU (带 KGDB 支持)
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

# 2. 开发机连接 GDB
aarch64-linux-gnu-gdb vmlinux
(gdb) target remote :5555
(gdb) break start_kernel
(gdb) continue
```

### QEMU KGDB 优势

| 优势 | 说明 |
|------|------|
| 无需物理串口 | 通过 TCP 连接 |
| 可调试启动早期 | 从第一条指令开始 |
| 可重复 | 快照 + 回放 |
| 无需硬件 | 开发机上完成 |

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

### HFT 关联

QEMU + KGDB 适合开发期调试内核模块——不需要每次部署到树莓派，快速迭代。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** QEMU KGDB 调试相比物理树莓派有什么优势？

> QEMU 通过 TCP 连接 GDB（无需物理串口线），可以从第一条内核指令开始调试（包括 head.S 汇编启动代码），支持快照回放（复现并发 bug），且在开发机上完成（无需切换设备）。缺点是性能不真实，无法测试真实硬件 I/O 延迟。

</details>
