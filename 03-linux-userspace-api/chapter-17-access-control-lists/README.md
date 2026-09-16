# TLPI 第 17 章 — Access Control Lists

**优先级**：🟡（多用户共享目录 / chmod↔mask 陷阱 / 权限版本化；概念价值 > 频率）
**前置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/README.md)（ACL 的存储载体）· [Ch15.4 权限](../chapter-15-file-attributes/README.md)（最小 ACL 的另一面）
**后置**：[Ch38 特权与安全](../chapter-38-secure-privileged/README.md)（file capabilities = `security.*` xattr 的邻居）

---

## 小节目录（与原书 17.1–17.10 一致）

- [17.1 Overview 概览](notes/17.1-overview.md) — 6 种 tag / 最小与扩展 ACL / 存储
- [17.2 ACL Permission-Checking Algorithm 判定算法](notes/17.2-acl-permission-checking-algorithm.md) ✅ 本机实测复现
- [17.3 Long and Short Text Forms 文本形式](notes/17.3-long-and-short-text-forms-for-acls.md)
- [17.4 The ACL_MASK Entry and the ACL Group Class](notes/17.4-the-acl-mask-entry-and-the-acl-group-cla.md) — mask 与 chmod 的兼容协议
- [17.5 The getfacl and setfacl Commands](notes/17.5-the-getfacl-and-setfacl-commands.md)
- [17.6 Default ACLs and File Creation](notes/17.6-default-acls-and-file-creation.md) — 新文件模板 / 忽略 umask
- [17.7 ACL Implementation Limits](notes/17.7-acl-implementation-limits.md)
- [17.8 The ACL API](notes/17.8-the-acl-api.md) — libacl / 两遍遍历
- [17.9 Summary 本章总结](notes/17.9-summary.md)
- [17.10 Exercise 练习](notes/17.10-exercise.md) — 17-1 listacls（Linux 复测）

---

## 章节目标

- **判定算法肌肉记忆**：①属主 ②named(∩mask) ③组类(OR∩mask) ④other——mask 约束②③不碰①④（c17_1 九用例实测）
- **mask 的兼容协议**：chmod 组位 = mask；`ls -l` 的 group 位在有 mask 时显示 mask；`+` 后缀的含义
- **default ACL**：目录的新文件模板，AND 语义、忽略 umask、随子目录递归——共享目录权限的正确姿势
- **诚实边界**：POSIX draft ACL 为 Linux 专有；本机可实测的是判定算法纯逻辑复现，真机行为（getfacl/setfacl/default 继承）列 Pi5 复测清单

---

## 原书示例清单（TLPI dist `acl/`，逐字镜像）

| Listing | 文件 | 说明 |
|---------|------|------|
| **17-1** | `acl_view.c` | 读并打印 ACL（getfacl 教学微缩版） |
| 补充 | `acl_update.c` | 增量修改 ACL（setfacl 教学微缩版） |

自编：`c17_1_acl_algorithm.c`（判定算法全平台复现，本机实测）· `ex17_1_listacls.c`（17-1，Linux 复测）。清单见 [`code/README.md`](code/README.md)。

---

## 一条主线：**ACL 不是"更多权限位"，而是一套带全局约束（mask）的判定协议**

看懂 17.2 的四步算法，17.4 的 mask、17.3 的 #effective 注释、`ls -l` 的 `+` 全部自洽；看不懂算法，工具用得再多也是玄学。
