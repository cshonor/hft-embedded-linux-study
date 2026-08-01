# Ch 1 §1 入门指南 (Getting Started)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 1. 入门指南 (Getting Started)

### 配置与编译

拿到内核源码后的典型第一步：

```bash
make config          # 交互式回答 Kconfig 选项（旧书时代常用）
make bzImage         # 编译可引导内核镜像
make modules         # 编译可加载模块
```

现代树更多用 `make menuconfig` / `make olddefconfig`；**本书成书较早**，命令名以原书为准，**流程不变**：**先配置 → 再编内核 + 模块**。

### 信息获取渠道

| 资源 | 用途 |
|------|------|
| 源码顶层 **`README`** | 版本说明、最简编译提示 |
| **`Documentation/`** | 官方文档树（含 `CodingStyle`、`SubmittingPatches`） |
| [LWN.net](https://lwn.net/) **Kernel Page** | 内核动向、深度文章 |
| **Kernelnewbies** | 新手向 FAQ、术语 |
| **LKML**（Linux Kernel Mailing List） | 补丁讨论主战场 |
| **Kernel Traffic**（及类似摘要站） | 邮件列表摘要，降低订阅全量 LKML 的成本 |

---
