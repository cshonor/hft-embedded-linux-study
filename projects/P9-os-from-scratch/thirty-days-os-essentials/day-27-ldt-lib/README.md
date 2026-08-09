# Day 27 · LDT 与库


> **原书第二十七章** · **安全 + 工程化** — 修 **ncst** 关闭、**LDT** 防 **crack7**、API **.obj 拆分**、**`apilib.lib`**、目录与 **`app_make.txt`**。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① ncst 修复** | **Shift+F1 / ×** 可关 | 先 **藏窗** 再后台释放 |
| **② LDT** | 每 task **局部段表** | **crack7** 跨 app 读写 **阻断** |
| **③ 拆分 API** | 一函数一 **.obj** | **hello3** 不再 **520B 虚胖** |
| **④ Library** | **`apilib.lib`** | Librarian 打包 |
| **⑤ 工程重构** | **haribote/apilib/apps** · **`app_make.txt`** | **`make run_full`** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 修复 ncst 关闭 | [notes/section-27.1-修复-ncst-关闭.md](./notes/section-27.1-修复-ncst-关闭.md) |
| LDT | [notes/section-27.2-LDT.md](./notes/section-27.2-LDT.md) |
| 拆分 API 目标文件 | [notes/section-27.3-拆分-API-目标文件.md](./notes/section-27.3-拆分-API-目标文件.md) |
| 库（Library） | [notes/section-27.4-库Library.md](./notes/section-27.4-库Library.md) |
| 整理 Make | [notes/section-27.5-整理-Make.md](./notes/section-27.5-整理-Make.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| ncst 关不掉？ | **统一 kill 路径** · **先 hide** |
| crack7 说明啥？ | **GDT 不够 app 互防** |
| LDT 干嘛？ | **每 task 私有段表** |
| hello3 为何大？ | **整包 API asm** → **按 .obj 链接** |
| 碎 obj 怎么管？ | **`apilib.lib`** |
| 工程？ | **haribote/apilib/apps** + **`app_make.txt`** |

---

---

## 本日学习目标 · 自检

- [ ] 对比 **GDT（OS/全局）vs LDT（单 app）**
- [ ] 说清 **静态库按需链接** 与体积关系
- [ ] 知道 **`apilib.lib`** 在构建链位置
- [ ] 描述 **ncst 关闭 UX（先藏后释）**
- [ ] 串 Day 21→25→27 **安全演进**

---

← [Day 26](./day-26-为窗口移动提速.md) · [01 导读](../README.md) · [Day 28](./day-28-文件操作与文字显示.md)

---

## 相关

- 上一日：[../day-26-window-speed/](../day-26-window-speed/)
- 下一日：[../day-28-files/](../day-28-files/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
