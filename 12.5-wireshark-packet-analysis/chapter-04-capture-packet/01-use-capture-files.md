# 4.1 使用捕获文件

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：捕获完成后通过保存、导出、合并管理 pcap，为对比分析与归档做准备。

## 核心知识点

### 保存与导出格式

| 格式 | 说明 |
|------|------|
| **`.pcapng`** | Wireshark **默认**；支持多接口、注释、自定义选项等**增强元数据** |
| **`.pcap`** | 传统 libpcap；兼容性好，元数据能力弱于 pcapng |
| **文本 / CSV / XML 等** | `File` → `Export Packet Dissections` → 供 Excel、脚本或第三方工具处理 |

**pcapng vs pcap（速记）**

| 项 | pcapng | pcap |
|----|--------|------|
| 多接口同文件 | ✅ 每接口独立链路类型 | 较难表达 |
| 抓包注释、方向 | ✅ | ❌ |
| 旧版工具 | 部分需升级 | 广泛支持 |

### 精简捕获文件（Export Specified Packets）

面对体积过大的「胖」文件：

| 菜单 | 用途 |
|------|------|
| `File` → `Export Specified Packets` | 只导出**当前显示**（过滤器后）、**全部**、**标记**、**选中**范围 |

→ 减小体积、聚焦证据包、便于邮件/工单附件。

### 合并捕获文件（Merge）

| 场景 | 操作 |
|------|------|
| 两端各抓一段、需对照同一事务 | `File` → `Merge` |
| 排序 | 按时间戳 **Chronologically** 合并；也可追加到当前文件 |

> 合并前注意各文件**时钟是否同步**；不同步时用 [§4.3 时间偏移](./03-time-display-format.md)。

## 抓包/实操记录

| 练习 | 步骤 |
|------|------|
| 导出子集 | 显示过滤器 `http` → Export Specified → Displayed only |
| 合并 | 抓 `client.pcapng` + `server-mirror.pcapng` → Merge chronologically |
| 归档命名 | `site-YYYYMMDD-baseline.pcapng` / `incident-ticketID.pcapng` |

## 疑问与总结

- 导出「Displayed」不会删除原文件；原胖文件仍保留完整数据。
- 合并后应用 **Time Reference** 或 **Time Shift** 对齐多探针时间线。
