## ① 获取内核源码 · Obtaining the Kernel Source

#### Git（推荐）

内核社区 **强烈推荐 Git** 管理源码树：

```bash
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux
git pull    # 跟进主线更新
```

| 方式 | 优点 |
|------|------|
| **Git** | 易更新 · 易打补丁 · 易切分支/tag · 与社区工作流一致 |

#### 压缩包

也可从 [kernel.org](https://www.kernel.org/) 下载 **bzip2 / gzip** 源码包并解压。

| 建议 | 原因 |
|------|------|
| **放在 `~/` 等用户目录** | 开发不必 root |
| **不要解压到 `/usr/src/linux`** | 避免污染系统树、误链系统头文件 |

#### 使用补丁 · Patches

社区交流以 **patch** 为通用语言：

```bash
cd linux
patch -p1 < ../patch-x.y.z
```

`-p1` 剥掉补丁路径前缀一层，与 `git am` / `git apply` 同属日常工具链。

→ 收官：[Ch](../../chapter-20-patches-community/)

---
