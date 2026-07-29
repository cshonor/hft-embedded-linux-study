## ① 获取内核源码 · Obtaining the Kernel Source

> **先看版本结论，再看怎么下、怎么验。** 本机已验收通过的是 **linux-7.1.5**（不是 7.0，也不是「跟书一模一样的 2.6」）。

---

### 一、该下哪个版本？（写进笔记的定论）

| 角色 | 版本 | 说明 |
|------|------|------|
| **书本对照版（LKD 3rd 官方）** | **Linux 2.6.34** | Love 第三版写到 **2.6.34**；并尽量与企业长线 **2.6.32** 事实一致（2010） |
| **本仓库推荐 · 日常主树** | **当前稳定版 7.1.x**（本机：**7.1.5**） | 读 `kernel/` `mm/` `net/`、嵌入式/HFT 直觉都看现代树 |
| **可选考古树** | **2.6.34** 再下一份 | 仅当「书上点名符号，7.x 对不上」时打开对照 |
| **不推荐为读 LKD 专门跟** | mainline **rc**（如 7.2-rc*） | 变动快；学习用 **stable** 即可 |

**关系一句话：**

```
LKD 书 ≈ 2.6.34 时代的地图（概念仍值钱）
你的主源码树 = 7.1.5（现代实现；目录名大多还在，细节会变）
不要指望 7.1.x 与书中每一行函数名一一对应
```

| 误解 | 纠正 |
|------|------|
| 「书是 2.6，所以必须只下 2.6」 | 学概念可以；**工程/驱动/HFT 主树用 7.1.x** |
| 「下了 7.1 就和书无关了」 | 顶层地图、CFS/中断/锁/mm **思想**仍通；只是实现演进了 |
| 「一定要 Git 全量 clone 才算对」 | 学习对照用 **固定版本 tar.xz** 往往更省事、更好验收 |

版本查询权威页：[kernel.org releases.json](https://www.kernel.org/releases.json)（看 `stable` 的 `version` / `source`）。

---

### 二、推荐下载顺序（按成功率）

在国内网络下，**不要一上来就全量 `git clone` torvalds 树**（体积大、易断）。读 LKD 的推荐顺序：

```
① 压缩包 tar.xz（国内镜像）  ← 首选：固定版本、好校验
② 需要跟进主线时再 Git（浅克隆 / blobless / 镜像）
③ （可选）另下一份 2.6.34 专供跟书对符号
```

#### 方式 A · 压缩包（本仓库实测首选）

| 步骤 | 做什么 |
|------|--------|
| 1 | 查当前 **stable** 版本号（本笔记记录时为 **7.1.5**） |
| 2 | 用 **国内镜像** 下 `linux-x.y.z.tar.xz`（tuna / ustc；官方 CDN 往往极慢） |
| 3 | 解压到 **用户目录**（`~/` 或 Windows `Desktop\`） |
| 4 | **不要** 解到 `/usr/src/linux` |
| 5 | 按下文 **验收清单** 核对 |

```bash
# 推荐：清华镜像 · 稳定版（版本号请按 releases.json 更新）
curl -LO https://mirrors.tuna.tsinghua.edu.cn/kernel/v7.x/linux-7.1.5.tar.xz
# 备用：中科大
# curl -LO https://mirrors.ustc.edu.cn/kernel.org/linux/kernel/v7.x/linux-7.1.5.tar.xz

tar -xf linux-7.1.5.tar.xz
cd linux-7.1.5
```

| 放置 | 原因 |
|------|------|
| 用户目录 | 开发不必 root |
| 禁止 `/usr/src/linux` | 避免污染系统树、误链发行版头文件 |

#### 方式 B · Git（社区正式推荐，但本机曾踩坑）

```bash
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux
git pull
```

| 优点 | 易更新 · 打补丁 · 切 tag/分支 · 与社区一致 |
|------|-----------------------------------------------|

**若 Git 失败，按本机经验降级：**

| 现象 | 对策 |
|------|------|
| kernel.org **HTTP 502** / `expected packfile` | 换镜像或改走 **tar.xz** |
| GitHub `Connection was reset` / `early EOF` | 加大 `http.postBuffer`；或改 **ustc/tuna 的 linux.git**；或放弃全量改 **tar** |
| 全量对象上千万、下半天仍断 | **不要硬刚全量**；学习用 tar 固定版 |
| 仍想要 Git | `--filter=blob:none` 或 `--depth 1`，再按需加深 |

可选（跟书对符号）：

```bash
curl -LO https://mirrors.tuna.tsinghua.edu.cn/kernel/v2.6/linux-2.6.34.tar.xz
tar -xf linux-2.6.34.tar.xz
```

---

### 三、本机实际下载经过（避免「下半天不知道对不对」）

| 尝试 | 结果 |
|------|------|
| `git clone` **kernel.org** torvalds | **失败**（HTTP 502） |
| `git clone` **GitHub** torvalds/linux 全量 | **失败**（连接重置 / early EOF） |
| GitHub **blobless** | **失败**（连接重置） |
| 官方 **cdn.kernel.org** tar.xz | 能连但 **极慢**（约几十 KB/s，不划算） |
| **tuna** `linux-7.1.5.tar.xz` | **成功**（约 151 MB = 158401920 字节） |
| Windows `tar -xf` | **大体成功**；`tools/testing/selftests/` 个别路径 `Invalid argument` |

**结论：** 对读 LKD / 对照 `kernel` `mm` `net`，**当前这棵 7.1.5 树可用、下对了。** selftests 噪点可忽略；要 100% 文件完整请到 **WSL/Linux** 再解一份。

---

### 四、本机路径与验收（7.1.5 · 已通过）

| 项 | 路径 / 值 |
|----|-----------|
| 压缩包 | `C:\Users\12392\Desktop\linux-7.1.5.tar.xz`（**158401920** 字节） |
| 解压树 | `C:\Users\12392\Desktop\linux-7.1.5\` |
| Makefile | `VERSION=7` `PATCHLEVEL=1` `SUBLEVEL=5`（即 **7.1.5**） |
| 绰号 | `NAME = Baby Opossum Posse` |

**打开树后请自检（有这些就说明「下好了、能读」）：**

```text
linux-7.1.5/Makefile
linux-7.1.5/include/linux/sched.h
linux-7.1.5/kernel/sched/core.c
linux-7.1.5/mm/mmap.c
linux-7.1.5/net/core/dev.c
linux-7.1.5/init/main.c          ← start_kernel 在这附近
linux-7.1.5/arch/arm64/          ← 嵌入式 ARM64
linux-7.1.5/drivers/base/core.c
```

顶层还应能看到：`arch` `drivers` `fs` `include` `kernel` `mm` `net` `init` `ipc` `lib` …

| 验收项 | 本机 |
|--------|------|
| 包大小 ≈ 151MB | ✓ |
| Makefile 版本 7.1.5 | ✓ |
| 上表关键文件存在 | ✓ |
| 读 LKD 所需子系统目录 | ✓ |
| selftests 100% 无报错 | ✗（Windows 已知限制，不影响主学习） |

---

### 五、补丁（社区语言）

```bash
cd linux-7.1.5   # 或 git 工作树
patch -p1 < ../some.patch
```

`-p1` 剥一层路径前缀；亦可用 `git apply` / `git am`。

→ 顶层目录导航：[§2.2](./section-2.2-内核源码树.md) · [KERNEL-SOURCE-TREE-MAP](../../KERNEL-SOURCE-TREE-MAP.md)  
→ 收官：[Ch 20](../../chapter-20-patches-community/)

---
