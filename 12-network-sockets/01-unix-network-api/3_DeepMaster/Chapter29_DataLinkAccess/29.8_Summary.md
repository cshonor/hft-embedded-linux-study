# 29.8 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 最高权限视野

数据链路访问 = 按**帧**观察/注入 — 嗅探、防火墙、发包工具的物理基础。

### 2. 内核过滤是生命线

**BPF 虚拟机**（BSD `/dev/bpf`、Linux `SO_ATTACH_FILTER`）— 避免百万帧淹没用户态。

### 3. 黄金法则

| 需求 | 库 |
|------|-----|
| **读（抓包）** | **`libpcap`** |
| **写（造包）** | **`libnet`** |

勿在各 OS 原生 DLPI/PF_PACKET 上重复造轮子（除非写库本身）。

### 4. 与 Ch 28

- Raw socket：**IP 层** ICMP 等  
- **tcpdump/Wireshark**：**Ch 29 + libpcap**

---

## 技术栈对照

```text
应用
  libpcap (read)     libnet (write)
       ↓                  ↓
  BPF / PF_PACKET / DLPI / ...
       ↓
  网卡（可混杂模式）
```

---

## 个人学习总结

（待填）
