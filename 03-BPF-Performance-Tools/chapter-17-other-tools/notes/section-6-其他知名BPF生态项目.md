# 6. 其他知名 BPF 生态项目

| 项目 | 定位 | HFT 关联 |
|------|------|----------|
| **Cilium** | K8s **BPF 网络 + 安全策略** | 容器集群 CNI；非裸金属 tick 路径 |
| **Sysdig** | 容器 **可观测性**（BPF 扩展） | 商业/开源混合 — 与 CLI BCC 互补 |
| **Android eBPF** | 移动端网络监控 | 本书外 |
| **osquery + eBPF** | 主机分析、kprobe 监控 | 安全/资产 — 偏 [Ch 11](../../chapter-11-security/) |
| **ply** | 类 bpftrace，**轻依赖**（无 LLVM/Clang） | **嵌入式** — 资源受限设备 |

### Cilium（略深）

- **eBPF 数据平面** — 替代 iptables 部分路径  
- **NetworkPolicy** — L3/L4/L7  
- **HFT：** 若交易 **不在 K8s Cilium 数据面**，仅作 **共置服务** 网络策略参考

### ply

| vs bpftrace | ply |
|-------------|-----|
| 依赖 LLVM/Clang | **更小依赖** |
| 功能/生态 | 较新、较轻 |

---
