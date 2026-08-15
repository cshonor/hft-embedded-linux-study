# 6. Host BPF 工具

### `kvmexits` — VM Exit 直方图 🔴

Host 上 bpftrace 工具：**VM exit 耗时分布** + **退出原因**。

```bash
sudo kvmexits.bt   # 或发行版包装脚本
```

| 退出原因（示例） | 常见解读 |
|------------------|----------|
| **`HLT`** | vCPU **空闲 halt** — 通常正常、耗时可长 |
| **`EXTERNAL_INTERRUPT`** | 外部中断注入 Guest |
| **`MSR_WRITE` / `MSR_READ`** | 模型特定寄存器访问 |
| **`EPT_VIOLATION`** | 嵌套页表 miss — MMIO/内存模拟 |
| **`IO_INSTRUCTION`** | 端口 I/O 模拟 — 设备路径慢 |

**价值：** Host 管理员 **快速定位异常 exit** — 某原因延迟尖刺 → 查 KVM/QEMU/设备模拟。

### Host 分析 Guest 的局限（Future Work）

| 能做 | 难做 |
|------|------|
| 获取 **Guest RIP**（退出时指令指针） | **无 Guest 符号表** → 难译成函数名 |
| 统计 exit 原因与延迟 | **进程级** 上下文在 Guest 内 |

**结论：** **深度应用/profile 仍在 Guest 内**；Host 侧 **`kvmexits`** 看 **虚拟化层** 健康度。

---
