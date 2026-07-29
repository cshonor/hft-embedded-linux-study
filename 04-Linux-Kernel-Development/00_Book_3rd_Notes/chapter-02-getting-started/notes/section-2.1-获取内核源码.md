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

也可从 [kernel.org](https://www.kernel.org/)（或国内镜像如 tuna/ustc）下载 **xz / gzip** 源码包并解压。

| 建议 | 原因 |
|------|------|
| **放在用户目录**（Linux `~/linux-*` · Windows 如 `Desktop\linux-*`） | 开发不必 root |
| **不要解压到 `/usr/src/linux`** | 避免污染系统树、误链发行版头文件 |

稳定版示例（版本号以 [releases.json](https://www.kernel.org/releases.json) 为准）：

```bash
# 示例：稳定版 tar.xz（镜像可换 tuna）
curl -LO https://cdn.kernel.org/pub/linux/kernel/v7.x/linux-7.1.5.tar.xz
tar -xf linux-7.1.5.tar.xz
cd linux-7.1.5
```

| 方式 | 优点 |
|------|------|
| **Git** | 易更新 · 补丁 · 分支/tag · 社区工作流 |
| **压缩包** | 网络不稳时更省事；固定版本好对照笔记 |

#### 使用补丁 · Patches

社区交流以 **patch** 为通用语言：

```bash
cd linux   # 或 linux-x.y.z 解压目录
patch -p1 < ../patch-x.y.z
```

`-p1` 剥掉补丁路径前缀一层，与 `git am` / `git apply` 同属日常工具链。

顶层目录导航 → [§2.2](./section-2.2-内核源码树.md) · [KERNEL-SOURCE-TREE-MAP](../../KERNEL-SOURCE-TREE-MAP.md)

→ 收官：[Ch 20](../../chapter-20-patches-community/)

---
