# 16.8 高速环境下的 TCP

> 章级精读：[../study.md#ch16-8](../study.md#ch16-8)

## 本节核心目标

理解 **LFN（长肥管道）** 问题及 **BIC/CUBIC**。

---

## LFN 问题

- 经典 Reno **线性增 cwnd** → 填满高 **BDP** 需数千 RTT

---

## BIC / CUBIC

- **CUBIC**（Linux 默认）：三次函数 `W(t)`  
  - 远离上次丢包点：**快增**  
  - 接近容量：**缓增** 再探测

---

## BBR（扩展）

- Google **BBR**：融合延迟/带宽估计；`sysctl net.ipv4.tcp_congestion_control=bbr`
- 弱网/高丢包跨国场景常优于默认 **cubic**（需实测）
