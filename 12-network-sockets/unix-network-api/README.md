# UNP Vol.1 — Unix Network Programming

**定位：** 用户态 Socket API（Stevens）。

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

- 实战：[12-network-sockets/muduo-sockets](../../12-network-sockets/muduo-sockets/)
- 协议：[13-tcpip-protocols](../../13-tcpip-protocols/)
