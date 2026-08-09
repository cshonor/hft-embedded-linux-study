## ④ 提交错误报告 · Bug Reports

| 必备信息 | |
|----------|--|
| **症状** | 发生了什么 |
| **系统输出** | dmesg、配置 |
| **完全解码的 Oops** | 若有（Ch 18 · `kallsyms`） |
| **稳定复现步骤** | |
| **硬件说明** | 架构、设备 |

| 发往 | **`MAINTAINERS` 中相关维护者** + **抄送 LKML** |

| 原则 | **能复现 > 猜测** · **完整 Oops > 截图一行** |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 一个好的内核 bug 报告应该包含什么？

<details><summary>答案</summary>

1) 内核版本（`uname -r`）；2) .config（配置）；3) 复现步骤（最小化）；4) dmesg/oops 日志；5) 硬件信息（CPU/内存/设备）；6) 是否能 bisect；7) 已尝试的排查。HFT 系统的内核 bug 报告还应包含：交易负载特征、时序信息（延迟突增时间点）、NUMA 配置、实时补丁版本。

</details>

</details>
---
