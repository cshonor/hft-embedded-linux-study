# 4. BPF 工具：top 与 I/O 尺寸（biotop / bitesize）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.3.3–9.3.4 节（印刷 p392–396）

## biotop（BCC）

top 的磁盘版：按进程统计 IOPS / 字节 / 延迟，周期刷新。

关键局限：**后台写回的 I/O 归属是 kworker 内核线程**，看不到真正发起写的应用进程。要找原进程需回第 8 章（cachestat/writeback/filetop 在页缓存层还有进程上下文）——这是"层越低、归因越模糊"的典型例子。

## bitesize（BCC / BT）

按**进程**统计 I/O **尺寸**直方图：跟踪 `block_rq_issue`，取 `args->bytes` 和 `args->comm`。

怎么用：

| 负载类型 | 看什么 |
|----------|--------|
| 顺序负载 | 关注**最大尺寸**档（越大越接近顺序满速） |
| 随机负载 | 让**应用 I/O 尺寸匹配设备**记录尺寸（如 4K 对齐） |

小尺寸高频率 I/O = 每次只搬一点，是常见优化点（对照第 8 章 vfssize 抓 grpc 26 万次 1 字节读的套路，这里是块层版）。

## HFT 关联

- 行情落盘线程若出现大量 512B/1KB 小写，说明写缓冲/对齐有问题——bitesize 一眼看出
- biotop 只能到 kworker 层，定位"哪个策略在打盘"要配合第 8 章 filetop

## 常见陷阱

- biotop 里看到 kworker 就下结论"内核自己写的"——实际是某应用的脏页写回
- 顺序负载看平均尺寸不看最大档——被少量合并大 I/O 拉高平均而误判

<details>
<summary>自测题</summary>

1. biotop 为什么看不到后台写的原进程？怎么办？
2. bitesize 对顺序负载和随机负载分别关注什么？

</details>
