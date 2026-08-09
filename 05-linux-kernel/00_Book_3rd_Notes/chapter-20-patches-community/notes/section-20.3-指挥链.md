## ③ 指挥链 · Chain of Command

| 文件 | **`MAINTAINERS`**（源码树根目录） |
|------|-----------------------------------|
| 内容 | 各 **驱动/子系统维护者** 名单与范围 |

| 顶层 | **Linus** — **主线（mainline）** 树最终维护者 |

```
你的网卡驱动补丁
    ▼
查 MAINTAINERS → netdev 维护者
    ▼
发邮件（非直接乱投 Linus，除非小全局改动）
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** MAINTAINERS 文件的作用？补丁如何从贡献者到 Linus？

<details><summary>答案</summary>

MAINTAINERS 记录每个子系统的维护者（名字/邮箱/状态/SCM/审计状态）。补丁路径：贡献者 → 子系统维护者 → 子系统树 → linux-next（集成测试）→ Linus（mainline merge window）。层层 review 保证质量。HFT 公司如果有内核修改，需要找对子系统维护者提交，不能直接发 Linus。

</details>

</details>
---
