# 29.4 Linux：SOCK_PACKET 和 PF_PACKET

---

## Linux 演进

### 1. `SOCK_PACKET`（已淘汰）

```c
socket(AF_INET, SOCK_PACKET, htons(ETH_P_ALL));
```

- 早期**无内核过滤** → 混杂模式下**每帧**进用户态 → 性能灾难  

### 2. `PF_PACKET`（主流）

```c
socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
```

---

## Linux Socket Filter（LSF）

与 **BPF 兼容** 的过滤：

```c
setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, ...);
```

将编译好的 **BPF 字节码** 挂到套接字 → 接近 BSD BPF 性能。

---

## 个人学习总结

（待填）
