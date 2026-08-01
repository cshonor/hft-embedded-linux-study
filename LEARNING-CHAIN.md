# 学习链路摘要

> **定稿执行顺序：** [LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md)  
> **编号 = 读序**；顶层为纯技术模块名。

```
Phase1  00  digital-logic-cpu
Phase2  01  c-language → 02 computer-systems
Phase3  04  linux-userspace-api（穿插 05 os-from-scratch / 06 cpp）
Phase4  07  linux-kernel + 09 linux-mm（08 可后补）
Phase5A 10 → 11 → 12 → 13（14 兴趣）
Phase5B 15 → 16 → 17 → 18 → 19 → 20 → 21
Phase6  03 · 08 · 22 · 23 ·（兴趣）14
```

| # | 模块 | Phase |
|---|------|-------|
| **00** | [digital-logic-cpu](./00-digital-logic-cpu/) | **1** 当前 |
| **01** | [c-language](./01-c-language/) | **2** |
| **02** | [computer-systems](./02-computer-systems/) | **2** |
| **03** | [computer-architecture](./03-computer-architecture/) | **6** 拓展 |
| **04** | [linux-userspace-api](./04-linux-userspace-api/) | **3** |
| **05** | [os-from-scratch](./05-os-from-scratch/) | **3** 穿插 |
| **06** | [cpp](./06-cpp/) | **3** 穿插 |
| **07** | [linux-kernel](./07-linux-kernel/) | **4** |
| **08** | [linux-kernel-deep](./08-linux-kernel-deep/) | **6** 拓展 |
| **09** | [linux-mm](./09-linux-mm/) | **4** |
| **10–14** | ARM → 构建 → 驱动/DT → 实战 → 飞控 | **5A** |
| **15–21** | Socket → TCP/IP → 内核网 → DPDK → 性能 → BPF → HFT | **5B** |
| **22–23** | Rust 量化 · 市场微观结构 | **6** |

**00 深度：** 黑盒为主，见 [00 学习深度](./00-digital-logic-cpu/学习深度_时序对Linux驱动.md) · [CSAPP↔数字逻辑](./00-digital-logic-cpu/学习路线_CSAPP与Harris_Linux驱动.md)
