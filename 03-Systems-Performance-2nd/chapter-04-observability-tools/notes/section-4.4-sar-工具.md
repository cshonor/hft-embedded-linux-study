## 4.4 sar 工具

**sar（System Activity Reporter）** — 虽有了 BPF，仍是**必备**传统利器。

| 能力 | 说明 |
|------|------|
| **实时** | `sar -u 1`、`sar -n DEV 1`、`sar -B 1` 等 |
| **历史** | 后台 **sadc** 定期采样，`sar -f` 读归档 |
| **覆盖** | CPU、内存、swap、I/O、网络、队列、进程… |

**为何仍重要：**

- 低开销、久经考验
- **「上周同一时段对比」** — BPF 常缺长期基线 unless 自建
- 与 [USE 方法](../../appendix-A-USE方法Linux.md) 清单字段高度重合

**常用示例：**

```bash
sar -u 1 5          # CPU
sar -n DEV 1 5      # 网络接口
sar -q 1 5          # 运行队列与 load
sar -r 1 5          # 内存
sar -B 1 5          # 分页统计
```

**HFT：** 热路径机器 **sadc 间隔别太短**（如 ≥10s）；危机时用实时 `sar`，复盘用归档。

→ 字段详解：[附录 B sar 总结](../../appendix-B-sar总结.md)

---


---

← [本章导读](../README.md)
