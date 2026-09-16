# TLPI 第 16 章 — Extended Attributes

**优先级**：🟡（ACL / capabilities / SELinux 标签的底层载体；篇幅短但概念常被混淆）
**前置**：[Ch15 File Attributes](../chapter-15-file-attributes/README.md)（inode 模型 + TOCTOU 模式）
**后置**：[Ch17 Access Control Lists](../chapter-17-access-control-lists/README.md)（ACL 存在 `system.*` xattr 里）· [Ch38 特权与安全](../chapter-38-secure-privileged/README.md)（file capabilities = `security.capability`）

---

## 小节目录（与原书 16.1–16.5 一致）

- [16.1 Overview 概览](notes/16.1-overview.md) — namespace 规则表（xattr 的宪法）
- [16.2 Implementation Details 实现细节](notes/16.2-extended-attribute-implementation-detail.md) — 存储布局 / 大小限制 / inode 竞态
- [16.3 System Calls 系统调用](notes/16.3-system-calls-for-manipulating-extended-a.md) — 六调用 + 探大小 + CREATE/REPLACE
- [16.4 Summary 本章总结](notes/16.4-summary.md)
- [16.5 Exercise 练习](notes/16.5-exercise.md) — 16-1 简易 setfattr ✅ 实测

---

## 章节目标

- **分清三层**：权限位（VFS 判定）/ i-node flags（FS 行为开关）/ **xattr（应用元数据）**——名字像、机制远；
- **namespace 规则**：只有 `user.*` 对非特权应用完全开放；`security./trusted.` 的特权检查是 Linux 实现行为，macOS 不强制（实测）——跨平台只依赖 `user.*`；
- **接口肌肉记忆**：buf=NULL 探大小、listxattr 是 NUL 拼接串、CREATE/REPLACE 语义、ENOATTR(93)/ENODATA(61) 两宏都判；
- **fd 化纪律**：set/get xattr 走 fd 版，杜绝 stat-then-set 的 TOCTOU（实测 0444 文件拒写也证明了 xattr 不是权限后门）。

---

## 原书示例清单（TLPI dist `xattr/`，逐字镜像）

| Listing | 文件 | 说明 |
|---------|------|------|
| **16-1** | `xattr_view.c` | 列出并打印文件全部 xattr（读侧） |
| 补充 | `t_setxattr.c` | setxattr 演示（**5 参 Linux 签名**，仅 Linux 可编） |

自编 demo `c16_1_xattr_basic.c` / `c16_2_namespaces.c` 与习题 `ex16_1_setfattr.c` 全部在本机（macOS / clang 23.1.0）编译零警告、运行成功，输出钉进各节笔记；用法见 [`code/README.md`](code/README.md)。

---

## 一条主线：**xattr 是 inode 上的小数据库，不是另一个存储引擎**

它复用文件的权限模型（读写 xattr = 读写文件），复用 inode 的 cache 亲和（小属性随 inode 常驻），也复用 inode 的竞态面（fd 版才是稳的）。看懂"复用"二字，本章所有细节都顺理成章。
