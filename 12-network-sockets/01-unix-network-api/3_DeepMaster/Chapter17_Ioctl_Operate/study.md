# 第 17 章：ioctl 操作（厚版）

> [阶段二 Ch 26](../../2_AdvancedSkill/Chapter26_Thread/study.md) · **Ch 17**（`3_DeepMaster`）· [Ch 18](../../4_ArchitectureDesign/Chapter18_RoutingSocket/study.md)  
> 逐节：`17.x_*.md`

> **说明**：上传资料截至第 8 章；第 17 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

理解 **`ioctl` 原型**、套接字/文件命令、**SIOCGIFCONF 截断陷阱**、**get_ifi_info**、单接口与 ARP 操作，以及为何路由应改用 **Ch 18 路由套接字**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 17.1 | [17.1_Overview](./17.1_Overview.md) | 四大领域与 POSIX 替代 |
| 17.2 | [17.2_ioctl_Function](./17.2_ioctl_Function.md) | 原型与 request |
| 17.3 | [17.3_Socket_Ioctl_Operate](./17.3_Socket_Ioctl_Operate.md) | SIOCATMARK、属主 |
| 17.4 | [17.4_File_Ioctl_Operate](./17.4_File_Ioctl_Operate.md) | FIONBIO、FIONREAD |
| 17.5 | [17.5_Network_Interface_Config](./17.5_Network_Interface_Config.md) | ifreq/ifconf、SIOCGIFCONF |
| 17.6 | [17.6_Get_Ifi_Info_Func](./17.6_Get_Ifi_Info_Func.md) | **get_ifi_info** |
| 17.7 | [17.7_Interface_Control_Operate](./17.7_Interface_Control_Operate.md) | SIOCGIF* / SIOCSIF* |
| 17.8 | [17.8_ARP_Cache_Operate](./17.8_ARP_Cache_Operate.md) | ARP 表 |
| 17.9 | [17.9_Route_Table_Operate](./17.9_Route_Table_Operate.md) | 勿用于新代码 |
| 17.10 | [17.10_Summary](./17.10_Summary.md) | 全章收束 |

---

## 一章速记

```text
ioctl(fd, request, arg)：无类型安全，优先 fcntl/getsockopt 专用 API
FIONREAD：排队字节数；FIONBIO→用 fcntl O_NONBLOCK 更规范
SIOCGIFCONF：缓冲区加倍直到 ifc_len < 容量；防静默截断
get_ifi_info：ifi_info 链表，多播/代理基石
路由：SIOCADDRT/DELRT 已过时 → AF_ROUTE（Ch18）
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 7.11 | fcntl 替代 FIONBIO、属主 |
| Ch 14.7 | FIONREAD |
| Ch 16 | 非阻塞 |
| Ch 18 | 路由套接字、get_ifi_info 另一实现 |
| Ch 20–21 | 多播需本机接口列表 |
