# Day 26 · 为窗口移动提速


> **原书第二十六章** · **GUI 性能 + Shell 增强** — **32 位写 VRAM**、**new_mx/new_my 延迟刷新**、**Shift+F2** 多 Console、**`start` / `ncst`**。

---

### 本节三段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 移动提速** | **32 位写** · **FIFO 空才刷** | 窗/鼠 **嗖嗖** |
| **② Console 管理** | 开机 **1 窗** · **Shift+F2** · **× 关 Console** |
| **③ start/ncst** | 新窗跑 app · **无黑框** 启动 | 孤立 app **安全退出** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 极致优化 | [notes/section-26.1-极致优化.md](./notes/section-26.1-极致优化.md) |
| 完善命令行窗口 | [notes/section-26.2-完善命令行窗口.md](./notes/section-26.2-完善命令行窗口.md) |
| start 与 ncst | [notes/section-26.3-start-与-ncst.md](./notes/section-26.3-start-与-ncst.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 绘图怎么 4×？ | **4 字节对齐处 32 位写** |
| 拖窗为何还卡？ | 每事件 refresh → **FIFO 空才刷** |
| 开机几个 Console？ | **默认 1** · **Shift+F2** 加窗 |
| 怎么关 Console？ | **× 可关** |
| `start`？ | **新 Console + 跑指定 app** |
| `ncst`？ | **无 Console 后台跑 GUI app** |

---

---

## 本日学习目标 · 自检

- [ ] 解释 **32 位写** 与对齐前提
- [ ] 描述 **new_mx/my + FIFO 空刷新**
- [ ] 区分 **`start` vs `ncst`**
- [ ] 说清 **ncst 为何改 API 路由**
- [ ] 串 Day 10→23→26 **refresh 优化链**

---

← [Day 25](./day-25-增加命令行窗口.md) · [01 导读](../README.md) · [Day 27](./day-27-LDT与库.md)

---

## 相关

- 上一日：[../day-25-multi-console/](../day-25-multi-console/)
- 下一日：[../day-27-ldt-lib/](../day-27-ldt-lib/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
