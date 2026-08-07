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


### 常见陷阱

1. sar 只看历史不告警——sar 记录历史但不主动告警，需要配合 Prometheus/Grafana 做实时告警
2. sar 默认粒度太粗——10 分钟平均看不到 HFT 微秒级尖刺，需要 sar -I 1 或更高频
3. 不存 sar 历史数据——出事才想看昨天的趋势，但 sadc 没配或被清理了

<details>
<summary>自测题（点击展开）</summary>

1. sar 的主要用途是什么？
   <details><summary>答</summary>历史性能数据回溯——记录 CPU/内存/IO/网络等指标，事后分析趋势</details>
2. sar 对 HFT 的局限性是什么？
   <details><summary>答</summary>默认 10 分钟粒度太粗，看不到微秒级尖刺；且只记录不告警</details>
3. 如何让 sar 数据对 HFT 有用？
   <details><summary>答</summary>调高采样频率（sar -I 1）+ 长期存储 + 配合实时监控（Prometheus）</details>

</details>


---

← [本章导读](../README.md)
