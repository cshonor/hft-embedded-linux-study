# 9. 中断与其他

### `softirqs` / `hardirqs`

测量处理 **软/硬中断** 的 **时间分布**（不仅是次数）— 网络、块设备高负载时内核态飙高的常见原因。

```bash
sudo hardirqs-bpfcc 5
sudo softirqs-bpfcc 5
```

### `smpcalls`

**SMP 跨核调用 (IPI)** 耗时 — 多核同步、TLB shootdown 等。

```bash
sudo smpcalls-bpfcc
```

### `llcstat`

利用 **硬件 PMC**，在内核汇总 **每进程 LLC 命中/未命中**。

```bash
sudo llcstat-bpfcc 5
```

**注意：** 需 PMU 可用；虚拟化环境可能受限。

---
