# 18.7 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 内核与用户态的异步桥梁

**`AF_ROUTE`** 打破路由表黑盒：

- 自定义路由协议（OSPF/BGP 守护进程）可写表项  
- **事件广播**让应用对链路断开、IP 漂移、ICMP 重定向等**毫秒级**响应  

### 2. 指针偏移的艺术

掌握 **`rt_msghdr` + `rtm_addrs` + 紧凑 sockaddr 群** 的步进解析 — 与 **`sockaddr_dl`** 的 `sdl_nlen`/`sdl_alen` 同属一类难点。

### 3. sysctl 全面胜出

相对 `ioctl` 的历史包袱与截断缺陷：

```text
先问长度 → malloc → 再取数据
```

是现代获取路由表、接口拓扑的**基石**。

### 4. get_ifi_info 双实现

| 章 | 实现 |
|----|------|
| Ch 17 | `ioctl` + SIOCGIFCONF |
| Ch 18 | **`sysctl` + NET_RT_IFLIST**（推荐理解方向） |

---

## 学习路径

```text
Ch17 ioctl 局限 → Ch18 AF_ROUTE + sysctl → Ch21 多播选接口
```

---

## 个人学习总结

（待填）
