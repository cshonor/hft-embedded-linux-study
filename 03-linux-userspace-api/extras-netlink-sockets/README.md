# TLPI 扩展 — Netlink Sockets（书中无独立章）

**优先级**：⭐ / ⭐⭐ / ⭐⭐⭐（见根目录 [README.md](../README.md) 优先级表）  

---

## 小节目录

- [00.3 要点梳理](./notes/00.3-section-00-3.md)

---

## 1. 本章目标




---

## 2. 核心 API / syscall




---

## 4. C 示例摘要




---

## 5. Rust 对照（`std` / `libc` / crate）




---

## 6. 常见坑与面试点




---

## 7. 背诵卡


| # | 要点 |
|---|------|
| 1 | |

---


---

## 8. 参考


- 《The Linux Programming Interface》第 55 章 — Netlink Sockets
- `man 2` / `man 3` / `man 7`


---

## 代码示例

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>

/* Netlink 套接字 — 用户态与内核通信的专用机制。
 * 演示通过 NETLINK_ROUTE 获取网络接口信息。
 * 编译: gcc -o netlink_demo netlink_demo.c */

int main(void) {
    /* 创建 netlink 套接字 */
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) { perror("socket"); return 1; }

    /* 绑定本地地址 */
    struct sockaddr_nl local;
    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_pid = getpid();
    local.nl_groups = 0;
    if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("bind"); return 1;
    }

    /* 构造 RTM_GETLINK 请求: 获取所有网络接口 */
    struct {
        struct nlmsghdr hdr;
        struct ifinfomsg ifi;
    } req;

    memset(&req, 0, sizeof(req));
    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.hdr.nlmsg_type = RTM_GETLINK;
    req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.hdr.nlmsg_seq = 1;
    req.hdr.nlmsg_pid = getpid();
    req.ifi.ifi_family = AF_UNSPEC;

    /* 发送请求 */
    struct sockaddr_nl kernel;
    memset(&kernel, 0, sizeof(kernel));
    kernel.nl_family = AF_NETLINK;

    sendto(fd, &req, req.hdr.nlmsg_len, 0,
           (struct sockaddr *)&kernel, sizeof(kernel));

    /* 接收内核回复 */
    char buf[8192];
    int n = recv(fd, buf, sizeof(buf), 0);
    if (n < 0) { perror("recv"); return 1; }

    /* 解析 netlink 消息 */
    printf("Network interfaces:\n");
    struct nlmsghdr *nlh;
    for (nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, n); nlh = NLMSG_NEXT(nlh, n)) {
        if (nlh->nlmsg_type == NLMSG_DONE)
            break;

        struct ifinfomsg *ifi = NLMSG_DATA(nlh);

        /* 解析属性获取接口名 */
        struct rtattr *rta = IFLA_RTA(ifi);
        int attr_len = IFLA_PAYLOAD(nlh);

        char ifname[32] = "?";
        for (; RTA_OK(rta, attr_len); rta = RTA_NEXT(rta, attr_len)) {
            if (rta->rta_type == IFLA_IFNAME) {
                strncpy(ifname, RTA_DATA(rta), sizeof(ifname) - 1);
                break;
            }
        }

        printf("  ifindex=%d  name=%s  flags=0x%x\n",
               ifi->ifi_index, ifname, ifi->ifi_flags);
    }

    close(fd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
