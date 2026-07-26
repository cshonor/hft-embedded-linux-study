# 第 25 章：信号驱动式 I/O（厚版）

> [Ch 24](../Chapter24_OutOfBandData/study.md) · **Ch 25** · [Ch 26](../../2_AdvancedSkill/Chapter26_Thread/study.md)  
> 逐节：`25.x_*.md`

> **说明**：上传资料截至第 8 章；第 25 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 **SIGIO** 三步骤、**UDP vs TCP** 差异、SIGIO UDP 回射的**队列 + sigprocmask/sigsuspend** 架构，以及相对 epoll 的历史定位。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 25.1 | [25.1_Overview](./25.1_Overview.md) | 信号驱动 I/O 模型 |
| 25.2 | [25.2_Socket_Signal_Drive_IO](./25.2_Socket_Signal_Drive_IO.md) | 三步骤；UDP/TCP |
| 25.3 | [25.3_SIGIO_UDP_Echo_Server](./25.3_SIGIO_UDP_Echo_Server.md) | 队列、临界区 |
| 25.4 | [25.4_Summary](./25.4_Summary.md) | 全章收束 |

---

## 一章速记

```text
SIGIO：sigaction + F_SETOWN + O_ASYNC(F_SETFL)
UDP：报文到或异步错误 → recvfrom 明确
TCP：事件太多 → 几乎不用（listen 特例可 accept）
UDP 服：处理函数非阻塞抽干 recvfrom → 全局队列
主循环：sigprocmask 阻塞 SIGIO → 空则 sigsuspend → 临界区 dequeue
现代：epoll/kqueue 替代
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 6.2 | 五种 I/O 模型 |
| Ch 8 | UDP 迭代服 → SIGIO 版 |
| Ch 16 | 非阻塞 recvfrom 抽干 |
| Ch 5.8 | sigaction、信号 |
| Ch 14.9 | epoll 替代 |
| Ch 24 | SIGURG vs SIGIO（不同信号） |

---

## 3_DeepMaster 进度（部分）

| 章 | 状态 |
|----|------|
| 17、20–22、24、**25** | 厚版完成 |
| 18、23、28–29 | 待笔记 |
