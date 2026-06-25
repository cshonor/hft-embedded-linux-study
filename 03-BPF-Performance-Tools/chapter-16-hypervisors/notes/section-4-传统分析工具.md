# 4. 传统分析工具

Hypervisor **专用** 传统工具不多。

### KVM + perf（Host）

```bash
perf stat -e 'kvm:*' -a sleep 10
perf record -e kvm:kvm_exit -a sleep 10
perf script
```

| 事件 | 含义 |
|------|------|
| **VM exit** | Guest 操作需 Hypervisor 处理 |
| 退出原因 | HLT、IO、MSR、EPT violation… |

→ 与 **`kvmexits`** bpftrace 互补 — perf 更通用，bpftrace 更易 **直方图按原因分桶**。

### Guest 侧传统指标

| 来源 | 字段 |
|------|------|
| `top` / `mpstat` | **`%st` (steal)** |
| `/proc/stat` | `steal` jiffies |

---
