# 3.1 Wireshark 简史

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：追溯 Wireshark 的起源、Ethereal 更名经过及开源社区属性。

## 核心知识点

### 发展脉络

| 时间/节点 | 事件 |
|-----------|------|
| **1998** | Gerald Combs 因项目需要开发抓包工具，以 **Ethereal** 之名发布 |
| **许可证** | 采用 **GPL（GNU General Public License）** 开源 |
| **2006** | Combs 换雇主；原雇主持有 **Ethereal 商标**；团队未能续约 → 项目更名为 **Wireshark** |
| **此后** | Wireshark 社区快速壮大（合作开发者 **500+**）；原 Ethereal 分支停滞 |

### 关键结论

- Wireshark 是 **社区驱动** 的协议分析器，而非单一厂商闭源产品。
- **GPL** 要求衍生分发需遵守开源义务，促进全球贡献 **协议解析器（dissector）** 与功能补丁。

> **拓展**：新协议、私有协议常由社区提交 dissector；企业也可在内部分支维护私有解析插件（需注意 GPL 与链接方式）。

## 抓包/实操记录

| 操作 | 说明 |
|------|------|
| 查看版本与构建信息 | `Help` → `About Wireshark` |
| 查看贡献与文档 | 官网 [wireshark.org](https://www.wireshark.org/) · Wiki · 邮件列表 |

## 疑问与总结

- 命令行 **`tshark`** 与 GUI 同源，均来自 Wireshark 项目（Ethereal 时代亦有命令行传统）。
- 旧资料、旧实验环境仍可能写「Ethereal」——与 Wireshark 为同一血统。
