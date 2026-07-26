# UNP Vol1 — UNIX Network Programming 卷 1 学习目录

> 基于 **UNIX Network Programming, Volume 1, 3rd Edition**（Stevens / Fenner / Rudoff）  
> 三层浏览：**阶段 → 章节 → 小节 `.md` + `code/`**（节笔记平铺在章目录，少点两层）  
> **对照陈硕 PNP 课**：[PNP / UNP / C++ 三者关系](./PNP_UNP_CPP_三者关系.md) · [主题章节对照表](./PNP_UNP_CPP_三者关系.md#pnp-unp-map)

## 目录层级

```
UNP_Vol1/
├─ 1_BasicFoundation/          # 入门筑基期（Ch 1–8）
├─ 2_AdvancedSkill/              # 能力进阶期（Ch 11,13,14,16,26）
├─ 3_DeepMaster/                 # 深度精通期（Ch 17,20–22,24,25,28,29）
└─ 4_ArchitectureDesign/         # 架构拔高期（Ch 9–10,12,15,18–19,23,27,30–31）
```

## 命名规则

| 层级 | 格式 | 示例 |
|------|------|------|
| 一级 | `UNP_Vol1` | 根目录 |
| 二级 | `{序号}_{阶段英文名}` | `1_BasicFoundation` |
| 三级 | `Chapter{NN}_{章节英文名}` | `Chapter04_BasicTCPSocket` |
| 节笔记 | `{小节号}_{小节英文名}.md` | `4.3_Connect_Function.md` |

## 章目录结构

```
Chapter04_BasicTCPSocket/
├─ study.md              # 章级导读 / 索引
├─ 4.1_Overview.md       # 各节笔记（平铺，一眼扫全章）
├─ 4.2_Socket_Function.md
└─ code/
   └─ 4.3_Connect_Function/
      ├─ original_c/
      ├─ rewrite_go/
      └─ rewrite_rust/
```

## 节笔记模板（`.md` 文件）

```markdown
# 本节标题
## 核心知识点
## 关键函数与结构体
## 执行流程原理
## 易错点与坑点
## 个人学习总结
```

## 阶段与章节对照

| 阶段 | 章节 |
|------|------|
| **1_BasicFoundation** | 1 Introduction · 2 TCP/UDP/SCTP · 3 Socket Intro · 4 TCP Socket · 5 TCP Demo · 6 select/poll · 7 Socket Options · 8 UDP |
| **2_AdvancedSkill** | 11 Name/Address · 13 Daemon/inetd · 14 Advanced I/O · 16 Nonblocking I/O · 26 Threads |
| **3_DeepMaster** | 17 ioctl · 20 Broadcast · 21 Multicast · 22 Advanced UDP · 24 OOB · 25 Signal-Driven I/O · 28 Raw · 29 Datalink |
| **4_ArchitectureDesign** | 9–10 SCTP · 12 IPv4/6 · 15 Unix Domain · 18 Routing · 19 Key Mgmt · 23 Adv SCTP · 27 IP Options · 30 Design · 31 Streams |

## 维护

- 全书 **320** 个小节目录（Ch 1–31，不含 Exercises）
- 全部小节英文名以 `scripts/generate_structure.py` 内 `SECTIONS` 字典为准（四阶段全覆盖）
- 重新同步目录：`python scripts/generate_structure.py`（**不覆盖**已有节 `.md`；自动迁移旧小节文件夹并更新 `OUTLINE.md`）
- 若从旧三层目录升级：`python scripts/flatten_section_notes.py`（一次性；已跑过可忽略）

→ 完整树形索引：[OUTLINE.md](./OUTLINE.md)
