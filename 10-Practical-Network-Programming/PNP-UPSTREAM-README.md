# PNP — 陈硕《实用网络编程》实验目录

> **Practical Network Programming** · 实战踩坑，与 [UNP 卷 1](../UNP_Vol1/README.md) **对照学习**（非 UNP 配套课）  
> 关系说明：[PNP / UNP / C++ 三者关系](../UNP_Vol1/PNP_UNP_CPP_三者关系.md)

## 目录层级

```
PNP/
├─ README.md          # 本文件
├─ OUTLINE.md         # 实验模块大纲（按常见 PNP 主题）
├─ study.md           # 学习索引 / 进度表
└─ code/
   ├─ README.md       # 源码约定（对齐 UNP code/ 结构）
   └─ {实验名}/
      ├─ notes.md     # 本节坑点 + UNP 互链
      ├─ original_cpp/   # 课程 C++ 版（听课后粘贴）
      ├─ original_c/     # 底层 POSIX C（可选）
      └─ rewrite_rust/   # Rust 对照重写
```

## 与 UNP 的分工

| 轨道 | 放什么 |
|------|--------|
| **UNP_Vol1/** | Stevens 原书 API、系统化节笔记 |
| **PNP/** | 粘包、自连接、Netcat、TTCP、epoll 等 **实验 + 坑点笔记** |

Daytime 等 UNP Ch1 示例仍在 [UNP Ch1 code](../UNP_Vol1/1_BasicFoundation/Chapter01_Introduction/code/README.md)；PNP 从 **工程向实验** 起步。

## 推荐顺序

见 [OUTLINE.md](./OUTLINE.md) · 进度在 [study.md](./study.md) 勾选。

## 源码约定

与 UNP 一致：`original_cpp/` · `original_c/` · `rewrite_rust/` · `rewrite_go/`（占位）。  
Rust：`cargo run` 可跑；`/target/` 已 gitignore。
