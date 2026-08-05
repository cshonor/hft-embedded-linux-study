# 第 2 章 · 搭建树莓派实验环境

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **精读**  
> **本仓库：** 原书写 **Pi4B**；实机按 **Pi5** 适配 — 见下方笔记

---

## 本章定位

搭好 **交叉工具链 + QEMU(+GDB) +（可选）Pi5 串口**；BenOS 启动/链接流程跟原书学，**板上外设地址不要照抄 4B**。

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **适配总览** | [PI5-ADAPT.md](../PI5-ADAPT.md) |
| **实验主路径** | **QEMU** `-M virt -cpu cortex-a76`；Pi5 实物留给外设/ Linux OS |
| **代码** | [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice) |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-Pi5适配与实验路线.md](./notes/section-0-Pi5适配与实验路线.md) | **Pi5 vs 4B** · QEMU 优先 · 上板改基址/`config.txt` · 风险 |

---

## 本章 Checklist

- [ ] 读原书 Ch2（流程与 BenOS 结构）
- [ ] 按 Pi5 适配笔记：QEMU + `cortex-a76` 跑通一条调试链
- [ ] 明确：Ch3–20 架构实验默认 **不硬怼** Pi5 裸机外设

---

← [Ch 1](../chapter-01-arm64-fundamentals/) · 下一章 [Ch 3](../chapter-03-a64-load-store/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [全书总结](../BOOK-SUMMARY.md)
