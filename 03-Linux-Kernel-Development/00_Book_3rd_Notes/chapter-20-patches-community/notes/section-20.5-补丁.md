## ⑤ 补丁 · Patches

**所有内核修改以补丁形式流通** — 社区的通用语言。

#### 生成补丁

| 方式 | 命令/工具 |
|------|-----------|
| 经典 | **`diff -urN`** 对比 **未改树** vs **修改树** |
| **推荐** | **Git** |

```bash
# Git 工作流（书中 + 现代常态）
git commit -a -m "net: fix foo in bar driver"
git format-patch -1          # 为最近 1 个提交生成 0001-*.patch
git format-patch origin/main # 相对主线的一系列补丁
```

→ **Ch 2** `git clone` · `patch -p1` · **Ch 18** `git bisect`

#### 提交补丁

| 项 | 规范 |
|----|------|
| **收件人** | 相关 **维护者** + **抄送列表** |
| **主题** | **`[PATCH] brief description`** |
| **正文** | **技术说明 + 理由** · **基于的内核版本** |
| **形式** | 补丁 **inline 纯文本** 附在邮件末尾 |
| 大改动 | **拆成多个逻辑独立小补丁** |
| 提交后 | **耐心** · 回应 review · **发修订版** `[PATCH v2]` |

```
邮件结构：
  To: maintainer@...
  Cc: linux-kernel@vger.kernel.org
  Subject: [PATCH] net: fix race in foo

  说明段落（why / what / testing）

  ---
  diff --git a/...
  （补丁正文 inline）
```

| HFT 团队 | 内部驱动/内核 fork 仍建议 **小步 commit + format-patch 风格说明** — 便于 audit 与回滚 |

---
