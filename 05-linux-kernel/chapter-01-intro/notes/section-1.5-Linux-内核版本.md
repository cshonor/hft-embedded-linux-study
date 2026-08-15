## ⑤ Linux 内核版本

| 类型 | 说明 |
|------|------|
| **稳定版（Stable）** | 生产可用 |
| **开发版（Development）** | 新特性实验 |

**传统版本号**（如 **2.6.30.1**）：

| 段 | 含义（经典规则） |
|----|------------------|
| 次版本号 **第二位** | **偶数 → 稳定** · **奇数 → 开发** |

> 注：现行主线已长期 **3.x / 4.x / 5.x / 6.x**，「奇偶」规则 **概念仍常考**，但以 **kernel.org 分支政策** 为准。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 内核版本号 5.15.0-25-generic 中每段代表什么？

<details><summary>答案</summary>

5 = 主版本号（Linus 每次发布递增）；15 = 次版本号（偶数=稳定版，奇数=开发版，但 2.6 后此规则已废弃）；0 = 修订号（bug fix）；25 = 发行版补丁计数；generic = 发行版 flavor。HFT 生产应选 LTS（Long Term Support）版本。

</details>

**Q2.** 为什么 HFT 生产环境通常选择 LTS 内核而非最新版？

<details><summary>答案</summary>

LTS 内核有 2-6 年维护周期，bug fix 回溯稳定。最新版可能有性能改进但也引入回归。HFT 需要可预测的行为，LTS + 厂商验证补丁是安全选择。例如 PREEMPT_RT（实时补丁）通常需要 1-2 年才能从 mainline 回归到 LTS。

</details>

</details>
---
