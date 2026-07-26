# 第 18 章：路由套接字（厚版）

> [Ch 17 ioctl](../../3_DeepMaster/Chapter17_Ioctl_Operate/study.md) · **Ch 18**（`4_ArchitectureDesign`）· [Ch 19](../Chapter19_KeyManageSocket/study.md)  
> 逐节：`18.x_*.md`

> **说明**：上传资料截至第 8 章；第 18 章框架来自目录（约第 12 页），细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

掌握 **`AF_ROUTE` 路由套接字**、**`sockaddr_dl`**、**`rt_msghdr` 读写**、**sysctl Two-Pass**、**sysctl 版 get_ifi_info** 与 **if_nametoindex** 族 API。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 18.1 | [18.1_Overview](./18.1_Overview.md) | 三大能力、创建语法 |
| 18.2 | [18.2_DataLink_Socket_Addr](./18.2_DataLink_Socket_Addr.md) | **sockaddr_dl** |
| 18.3 | [18.3_Socket_Read_Write](./18.3_Socket_Read_Write.md) | **rt_msghdr**、rtm_addrs |
| 18.4 | [18.4_Sysctl_Operate](./18.4_Sysctl_Operate.md) | MIB、Two-Pass |
| 18.5 | [18.5_Get_Ifi_Info_Func](./18.5_Get_Ifi_Info_Func.md) | sysctl 版接口列表 |
| 18.6 | [18.6_Interface_Name_Index_Func](./18.6_Interface_Name_Index_Func.md) | if_nametoindex 等 |
| 18.7 | [18.7_Summary](./18.7_Summary.md) | 全章收束 |

---

## 一章速记

```text
socket(AF_ROUTE, SOCK_RAW, 0)；写需 root
进程写：RTM_ADD/DELETE…；内核广播路由变更（ioctl 做不到）
rt_msghdr + rtm_addrs → 紧凑 sockaddr，按 sa_len 步进解析
sysctl：CTL_NET, AF_ROUTE, …, NET_RT_DUMP / NET_RT_IFLIST
Two-Pass：oldp=NULL 取长度 → malloc → 再取
get_ifi_info：if_msghdr + ifa_msghdr 流；优于 Ch17 ioctl 版
应用用 if_nametoindex，少手写路由消息
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 17 | ioctl 局限、SIOCGIFCONF 截断、路由勿用 SIOC*RT |
| Ch 21 | 多播接口索引 |
| Ch 28–29 | 原始 / 数据链路套接字 |
| Ch 19 | 密钥管理套接字（同类“特殊域”） |
