# TLPI 第 15 章 — File Attributes

**优先级**：🔴（权限 / 安全 / 目录与链接编程基础；Ch38 特权程序与一切审计话题的地基）
**前置**：[Ch14 File Systems](../chapter-14-file-systems/README.md)（inode 模型）· [Ch8/Ch9 用户与凭证](../chapter-09-process-credentials/README.md)（属主判定的 ID 来源）
**后置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/README.md) · [Ch18 目录与链接](../chapter-18-directories-links/README.md)（nlink 的增减方）· [Ch38 特权与安全](../chapter-38-secure-privileged/README.md)（SUID 程序写法）

---

## 小节目录（与原书 15.1–15.7 一致）

- [15.1 Retrieving File Information: stat()](notes/15.1-retrieving-file-information-stat.md)
- [15.2 File Timestamps 文件时间戳](notes/15.2-file-timestamps.md)
  - §15.2.1 utime() / utimes() · §15.2.2 utimensat() / futimens()
- [15.3 File Ownership 文件属主](notes/15.3-file-ownership.md)
  - §15.3.1 新文件属主 · §15.3.2 chown()/fchown()/lchown()
- [15.4 File Permissions 文件权限](notes/15.4-file-permissions.md)
  - 判定算法 / access() / SUID·SGID·sticky / umask / chmod
- [15.5 I-node Flags](notes/15.5-i-node-flags-ext2-extended-file-attribut.md)（Linux 专有，源码核验）
- [15.6 Summary 本章总结](notes/15.6-summary.md)
- [15.7 Exercises 练习](notes/15.7-exercises.md)

---

## 章节目标

- **把 `struct stat` 从"背字段"升级到"能对账"**：`st_size/st_blocks/st_blksize` 三个大小各管一件事；三个时间戳谁动谁有实测矩阵；`(st_dev, st_ino)` 才是文件唯一键
- **权限判定算法**：三选一（属主→属组→其他）、不叠加——mode=0077 的文件属主读不了（实测）
- **TOCTOU 意识**：`access-then-open` 是错的；`open + fstat` 才对（实测 rename 后 fd 认老 inode）
- **umask 只管创建、chmod 豁免**（实测：0666 创建被剪成 0644，chmod 0666 后是 0666）
- **实测纠正的三条书外知识**：macOS 上 rename 动 ctime（Linux 不动）；BSD 新文件属组继承父目录（/tmp 下 gid=0）；no-op chown 的 ctime 更新 Linux 动、macOS 不动
- **诚实**：15.5 i-node flags 为 Linux 专有，本机（macOS）无法实测，仅源码核验 + 保留 dist 镜像，复测平台 Pi5

---

## 原书示例清单（TLPI dist `files/`，逐字镜像）

| Listing | 文件 | 说明 |
|---------|------|------|
| **15-1** | `t_stat.c` | stat/lstat 属性打印（配 `file_perms.{h,c}` 15-3/15-4） |
| **15-2** | `t_chown.c` | chown 演示（`-` = -1 = 不变；配 `ugid_functions.{h,c}`） |
| **15-5** | `t_umask.c` | umask 与 open/mkdir 的联合演示 |
| 补充 | `t_utime.c` / `t_utimes.c` | 15.2.1 的时间设置演示 |
| 补充 | `chiflag.c` | i-node flags 查改（**Linux 专有**） |

自编 demo `c15_1`–`c15_6` 与习题实现 `ex15_1/3/4/5/6` 全部在本机（macOS 26.6.2 / clang 23.1.0）编译零警告、运行成功；清单与用法见 [`code/README.md`](code/README.md)。

---

## 一条主线：**文件属性 = 内核 inode 的用户态投影**

`stat()` 不是"读文件"，是**读 inode**；`utimensat/chmod/chown` 也不是"改文件"，是**改 inode**。看懂这一层，三个现象立刻自洽：

| 现象 | inode 视角的解释 |
|------|-----------------|
| chmod 不动 mtime，只动 ctime | 内容在数据块，权限在 inode——改的是 inode，动 ctime 天经地义 |
| rename 不换 inode（Linux 不动文件戳） | 改的是目录项里的名字，inode 原地不动 |
| fstat 免疫路径竞态 | fd 直接绑 inode，绕过路径解析层 |
| 符号链接有自己的 stat | 链接是独立 inode，内容=目标路径串 |
