# UNP Vol.1 — Unix Network Programming

**文件夹 03.5** · 编号即读序 · [锁定路线](../README.md)

> **定位：** 用户态 Socket API（Stevens）— 03 用户态 API 的网络纵深。  
> **前置：** [03 Linux 用户态 API](../03-linux-userspace-api/)  
> **书目：** W. Richard Stevens *Unix Network Programming* Vol.1（PNP 之后）

## 本地两套内容

| 路径 | 来源 | 内容 |
|------|------|------|
| [`code/unpv13e/`](./code/unpv13e/) | [unpbook/unpv13e](https://github.com/unpbook/unpv13e) | **完整官方源码树**（编译用这个） |
| [`notes-from-Computer-Networking/`](./notes-from-Computer-Networking/) | `Desktop\Computer Networking\UNP_Vol1` | 你整理的章节笔记 + 少量 intro 示例 `.c` |

构建（WSL/Linux）：

```bash
cd code/unpv13e
./configure && make
```

## HFT 优先源码目录（unpv13e）

`intro` · `tcpcliserv` · `select` · `nonblock` · `sockopt` · `udpcliserv` · `mcast`

## 交叉阅读

- 实战（C++）：[04.5-network-sockets](../04.5-network-sockets/)
- 协议：[12-tcpip-protocols](../12-tcpip-protocols/)
- 抓包：[12.5-wireshark-packet-analysis](../12.5-wireshark-packet-analysis/)
