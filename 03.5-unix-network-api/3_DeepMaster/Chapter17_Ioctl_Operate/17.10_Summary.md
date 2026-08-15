# 17.10 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 历史遗物与现用利器

- 无类型安全的古老接口  
- 非阻塞、OOB、信号属主 → **`fcntl` / `sockatmark`**  
- **路由/ARP 动态管理** → **路由套接字 / Netlink**

### 2. 不可替代的拓扑探索

- **`SIOCGIFCONF`**：获取本机网卡列表的正统手段（许多环境）  
- **截断无报警** → 加倍缓冲区重试  
- **`get_ifi_info`**：封装为 `ifi_info` 链表

### 3. 底层特权干预

- **`ifreq`**：类 `ifconfig` 的接口配置  
- **`arpreq`**：类 `arp` 的静态/动态 ARP 管理（root 写操作）

---

## ioctl 使用决策树

```text
非阻塞/异步/OOB 属主？ → fcntl / sockatmark
本机有哪些网卡/IP？     → SIOCGIFCONF + get_ifi_info
改接口/ARP？            → ifreq/arpreq（常需 root）
动态路由？              → 勿 ioctl；用 Ch18 路由套接字
```

---

## 个人学习总结

（待填）
