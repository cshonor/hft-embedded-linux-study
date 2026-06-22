## 14.9 硬件延迟检测（hwlat）

### Hardware Latency Tracer

检测 **非 OS 可解释的长时间停顿** — 常见原因：

| 原因 | 说明 |
|------|------|
| **SMI** | 系统管理中断 — BIOS/固件 |
| **其他固件** | BMC、电源管理 |
| **硬件 bug** | 内存、PCIe |

```bash
TR=/sys/kernel/tracing
echo hwlat > $TR/current_tracer
echo 1 > $TR/tracing_on
sleep 30
echo 0 > $TR/tracing_on
cat $TR/trace
```

**何时用（HFT）：**

- **P99/P999 尖刺**；`perf`、`offcputime`、**biolatency** 都对不上
- CPU **非 idle 但 forward progress 停住**
- 裸机低延迟验收 — **hwlat baseline**

**对策：** 更新 BIOS、关 C-State/SMI 相关选项、换主板 — 与 [11-HFT ch05](../../../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/) 联动。

---


---

← [本章导读](../README.md)
