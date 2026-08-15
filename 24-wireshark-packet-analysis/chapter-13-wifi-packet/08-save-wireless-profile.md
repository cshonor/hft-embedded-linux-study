# 13.8 保存无线分析配置

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · Profile：[§3.4.6](../chapter-03-wireshark-intro/04-get-started.md)

**核心主旨**：列 + 过滤器 + 着色存为 **Wireless** Profile，有线/无线分析一键切换。

## 核心知识点

| 保存内容 | 说明 |
|----------|------|
| 自定义列 | RSSI、`wlan_radio.data_rate`、`wlan_radio.channel` |
| 显示过滤器按钮 | 常用 `wlan.fc.type == 0`、BSSID 等 |
| 着色规则 | Beacon / Auth 高亮（可选） |

**操作**：`Edit` → `Configuration Profiles` → 新建 **Wireless**；或右下角配置名下拉切换。

## 抓包/实操记录

| 步骤 | 操作 |
|------|------|
| 1 | 按 §13.6 加列 |
| 2 | 保存过滤器到工具栏 |
| 3 | Save as Wireless Profile |
| 4 | 切回 **Default** 做有线抓包 |

可打包 Profile 目录与团队共享（§3.4.6）。

## 疑问与总结

- 不同网卡接口名不同，Profile 中「默认接口」可能需各机改一次。
