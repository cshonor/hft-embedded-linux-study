# 第 15 章：Unix 域协议（厚版）

> [Ch 12](../Chapter12_IPv4_IPv6_Interop/study.md) · **Ch 15**（`4_ArchitectureDesign`）· [Ch 16](../../2_AdvancedSkill/Chapter16_NonBlockingIO/)（待笔记）  
> 逐节：`15.x_*.md`

> **说明**：上传资料截至第 8 章；第 15 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

掌握 **`AF_LOCAL`**、`sockaddr_un`、`socketpair`、流/报语义差异、**`SCM_RIGHTS` 传 fd** 与**凭证**辅助数据，以及 `unlink` 清理惯例。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 15.1 | [15.1_Overview](./15.1_Overview.md) | IPC、三大优势 |
| 15.2 | [15.2_UnixDomain_Socket_Addr](./15.2_UnixDomain_Socket_Addr.md) | **sockaddr_un** |
| 15.3 | [15.3_Socketpair_Func](./15.3_Socketpair_Func.md) | **socketpair** |
| 15.4 | [15.4_Socket_Basic_Func](./15.4_Socket_Basic_Func.md) | bind/unlink/权限 |
| 15.5 | [15.5_Unix_Stream_Client_Server](./15.5_Unix_Stream_Client_Server.md) | 字节流 |
| 15.6 | [15.6_Unix_Datagram_Client_Server](./15.6_Unix_Datagram_Client_Server.md) | 数据报 |
| 15.7 | [15.7_File_Descriptor_Transfer](./15.7_File_Descriptor_Transfer.md) | **SCM_RIGHTS** |
| 15.8 | [15.8_Sender_Credential_Receive](./15.8_Sender_Credential_Receive.md) | UID/GID 凭证 |
| 15.9 | [15.9_Summary](./15.9_Summary.md) | 全章收束 |

---

## 一章速记

```text
AF_LOCAL = AF_UNIX；地址是路径，bind 创建 socket 文件
同机 IPC：比 TCP loopback 快；可传 fd、可传凭证
bind 前 unlink；close 不删文件
socketpair + fork：父子全双工通道
SOCK_DGRAM：可靠、有边界；客户必须 bind 自己的路径
传 fd：sendmsg/recvmsg，SOL_SOCKET + SCM_RIGHTS
Master accept → Worker 处理（Nginx 模式）
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 4 | bind/listen/accept/connect |
| Ch 8 | UDP 对比（数据报） |
| Ch 14 | **sendmsg**、辅助数据、**cmsghdr** |
| Ch 13 | 守护进程 + socket 文件清理 |
| Ch 16 | 非阻塞 + `EWOULDBLOCK`（数据报 send） |
