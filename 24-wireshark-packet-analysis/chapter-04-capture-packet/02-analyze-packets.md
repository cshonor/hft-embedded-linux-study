# 4.2 分析数据包

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 界面：[§3.4 三面板](../chapter-03-wireshark-intro/04-get-started.md)

**核心主旨**：在海量包中通过查找、标记、打印快速定位并隔离关键数据。

## 核心知识点

### 1. 查找数据包（Find Packet）

`Edit` → `Find Packet`（或 `Ctrl+F`）

| 类型 | 用途 | 示例 |
|------|------|------|
| **Display filter** | 按显示过滤器表达式跳转/筛选 | `tcp.port == 443` |
| **Hex value** | 在**原始字节**中搜十六进制 | `00:ff` |
| **String** | 搜 ASCII 字符串（可选区分大小写） | `GET /index` |

- Display filter 模式：找到**满足条件的下一个包**。
- Hex/String：适合搜**载荷特征**、魔数、明文关键字。

### 2. 标记数据包（Mark Packet）

| 操作 | 效果 |
|------|------|
| 右键 → `Mark/Unmark` 或快捷键 | Packet List 中该包 **黑底白字** |
| 与导出配合 | `Export Specified` → **Marked packets only** |

**用途**：从海量流量中圈出「可疑会话首尾」「握手失败点」等，再单独导出或打印。

**技巧**：`Mark All Displayed` 可一次标记当前过滤器结果（版本菜单位置可能略有差异，以本地为准）。

### 3. 打印数据包（Print）

`File` → `Print`：

| 范围 | 场景 |
|------|------|
| 全部 / 选中 / 标记 / 显示结果 | 工单附件、离线评审 |
| 输出 | PDF 或打印机；可含 List / Details / Bytes |

## 抓包/实操记录

| 练习 | 目标 |
|------|------|
| 找 HTTP | String 搜 `HTTP/1.1` 或 Display filter `http` |
| 标记 RST | 过滤器 `tcp.flags.reset==1` → 标记首包 → 导出 Marked |
| 报告 | 打印 Marked + Summary 供非 Wireshark 同事阅读 |

## 疑问与总结

- **查找** ≠ **过滤器**：Find 多用于跳转；常驻筛选用显示过滤器栏。
- 标记只影响**显示与导出选择**，不改变包内容。
