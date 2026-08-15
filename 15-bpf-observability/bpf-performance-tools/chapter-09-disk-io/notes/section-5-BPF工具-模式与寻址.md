# 5. BPF 工具：模式与寻址（seeksize / biopattern）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.3.5–9.3.6 节（印刷 p396–399）

## seeksize（BCC / BT）

统计 I/O **请求**间的寻址距离直方图（旋转盘的机械代价），跟踪 `block_rq_issue`。

- 测的是**请求层**的随机度：合并前/调度器视角
- 验证案例：dd 顺序负载 → 29908 次偏移 0（纯顺序确认）
- **主要对旋转盘有意义**；SSD/NVMe 无磁头，寻址距离近似零成本

## biopattern（BCC / BT）

统计 I/O **完成**事件的随机/顺序比例：跟踪 `block_rq_complete`，输出 `%RND / %SEQ`。

- 与 seeksize 互补：一个看请求随机度（issue 侧），一个看完成随机度（设备实际服务侧）
- **转换案例**：某负载 %RND 83% → 调整为 100% SEQ 后**吞吐暴涨**——证明随机度是该瓶颈的主因

工具价值就在于此：把"感觉是随机 I/O"变成可度量的百分比，改动前后可对比。

## HFT 关联

- 交易/行情存储多为 NVMe，seeksize 意义有限；但 biopattern 仍用于验证日志文件是否被碎片化成随机写（多文件 interleaved append 会把顺序写变随机写）

## 常见陷阱

- 在 SSD 上跑 seeksize 得出"寻址距离大 = 有问题"的错误结论
- 混淆 issue 侧（请求随机度）与 complete 侧（完成随机度）——调度器/设备重排会让两者不同

<details>
<summary>自测题</summary>

1. seeksize 和 biopattern 各跟踪哪个跟踪点、各测什么侧的随机度？
2. 书中 %RND→%SEQ 转换案例说明了什么方法论？

</details>
