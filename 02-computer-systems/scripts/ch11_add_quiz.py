#!/usr/bin/env python3
"""
Ch11 网络编程 — 新手化批量改造脚本
7 个 section：替换空壳自测 → 常见陷阱(3) + 折叠自测题(4)
对无自测段的文件(11.4)：在导航行前插入
"""

import os

NOTES_DIR = os.path.join(os.path.dirname(__file__), "..", "chapter-11-network-programming", "notes")

SHELL_PATTERN = """### 口述巩固 · 自测

1. （待口述补）本节核心一句话？"""

SECTIONS = [
    # ── 11.1 客户端-服务器编程模型 (23行) ──
    {
        "filename": "section-11.1-客户端-服务器编程模型.md",
        "expand": None,
        "traps": [
            "**套接字本质是 fd** — socket 返回文件描述符，可用 read/write/close 操作，和普通文件统一接口",
            "**客户端主动 connect，服务器被动 accept** — 角色不同，API 调用顺序也不同",
            "**HFT 中行情客户端和订单网关角色不同** — 行情通常是 UDP multicast 接收方，订单是 TCP 客户端连券商",
        ],
        "quiz": [
            ("Q1: 客户端-服务器模型中，谁是主动方？谁是被动方？",
             "客户端主动发起连接（connect），服务器被动监听（listen）并接受连接（accept）。服务器先运行，等待客户端连接。"),
            ("Q2: 套接字（socket）在 Linux 中是什么？和文件描述符的关系？",
             "套接字是一种文件描述符（fd）。socket() 返回一个 int fd，可以用 read/write/close 操作，与普通文件统一接口。内核为 socket fd 维护发送/接收缓冲区。"),
            ("Q3: HFT 中常见的客户端-服务器角色有哪些？",
             "行情客户端连交易所 feed（UDP multicast 或 TCP）；订单网关作为 TCP 客户端连券商/交易所；风控/监控 HTTP admin API 作为 Web 服务器。"),
            ("Q4: 为什么说 socket 统一了网络和文件 I/O？",
             "Linux 一切皆文件。socket fd 和文件 fd 共享同一套系统调用（read/write/close），差异由内核处理。select/epoll 可同时监听文件和 socket fd。"),
        ],
    },

    # ── 11.2 网络 (18行, 薄) ──
    {
        "filename": "section-11.2-网络.md",
        "expand": """### 网络基础概念

- **LAN（局域网）** — 以太网、交换机；同一广播域
- **WAN（广域网）** — 路由器互联多个 LAN；互联网是最大的 WAN
- **协议分层（TCP/IP 四层模型）：**
  - **应用层** — HTTP、FIX、自定义二进制协议
  - **传输层** — TCP（可靠）/ UDP（低延迟）
  - **网络层** — IP（路由、寻址）
  - **链路层** — 以太网帧、ARP

| 层 | 数据单元 | HFT 关注 |
|----|----------|----------|
| 应用 | message | 协议解析效率 |
| 传输 | segment/datagram | TCP vs UDP 选择 |
| 网络 | packet | 路由跳数、MTU |
| 链路 | frame | 交换机延迟、NIC offload |

""",
        "traps": [
            "**协议分层不是为了慢，是为了解耦** — 每层只管自己的职责，上层不关心下层细节",
            "**HFT 关注所有层的延迟** — 链路层（NIC + 交换机）、网络层（路由跳数）、传输层（TCP 握手/Nagle）、应用层（解析）",
            "**以太网帧 MTU=1500** — 大于 MTU 的 IP 包分片，分片增加延迟和重组开销",
        ],
        "quiz": [
            ("Q1: TCP/IP 四层模型分别是什么？每层的数据单元叫什么？",
             "应用层（message）、传输层（segment/datagram）、网络层（packet）、链路层（frame）。"),
            ("Q2: LAN 和 WAN 的区别？交换机和路由器分别在哪个层？",
             "LAN 是局域网（同一广播域），用交换机（链路层）互联。WAN 是广域网，用路由器（网络层）互联多个 LAN。"),
            ("Q3: HFT 在网络延迟方面关注哪些因素？",
             "链路层：NIC 延迟、交换机跳数、ARP 缓存。网络层：路由跳数、MTU 大小。传输层：TCP vs UDP、Nagle 算法。应用层：协议解析效率。"),
            ("Q4: 以太网 MTU 是多少？超过会怎样？HFT 如何避免？",
             "MTU=1500 字节。超过则 IP 分片，增加延迟和重组开销。HFT 避免分片：控制包大小 < MTU，或用 UDP 而非 TCP（避免 MSS 协商）。"),
        ],
    },

    # ── 11.3 全球 IP 互联网 (17行, 近空) ──
    {
        "filename": "section-11.3-全球IP互联网.md",
        "expand": """### 全球 IP 互联网

- **IP 地址** — 32 位（IPv4），标识主机在网络中的位置
- **端口** — 16 位（0-65535），标识主机上的进程/服务
- **IP + 端口 = 套接字端点** — 唯一标识一个网络连接端点
- **DNS** — 域名 → IP 地址解析（如 `example.com` → `93.184.216.34`）

| 概念 | 说明 | HFT 注意 |
|------|------|----------|
| IPv4 | 32 位，4 字节 | 交易所通常用 IPv4 |
| IPv6 | 128 位，16 字节 | 逐渐普及，需双栈支持 |
| 端口 | 16 位 | 行情/订单有固定端口 |
| DNS | 域名→IP | HFT 启动时解析一次，缓存 IP |

**HFT：** DNS 解析有延迟波动，启动时预解析并缓存 IP 地址，热路径不用 DNS。

""",
        "traps": [
            "**IP 地址标识主机，端口号标识进程** — 两者结合才唯一标识网络连接端点",
            "**DNS 解析有延迟波动** — HFT 启动时解析一次缓存，热路径不用 DNS",
            "**IP 地址可能变化** — 交易所 IP 变更需运维通知，不能硬编码 IP 而不设监控",
        ],
        "quiz": [
            ("Q1: IP 地址和端口号分别标识什么？各多少位？",
             "IP 地址标识主机在网络中的位置（IPv4 32 位 / IPv6 128 位）。端口号标识主机上的进程/服务（16 位，0-65535）。IP + 端口 = 套接字端点。"),
            ("Q2: DNS 的作用是什么？HFT 如何处理 DNS？",
             "DNS 将域名解析为 IP 地址。HFT 启动时预解析并缓存 IP，热路径直接用 IP 连接，避免 DNS 查询的延迟波动。"),
            ("Q3: 为什么 HFT 不在热路径调用 DNS？",
             "DNS 查询走 UDP（可能多次重试），延迟不确定（μs-ms 级），且依赖 DNS 服务器可用性。热路径需要确定性延迟，不能容忍 DNS 波动。"),
            ("Q4: IPv4 和 IPv6 的主要区别？HFT 需要支持哪个？",
             "IPv4 32 位地址（约 43 亿），IPv6 128 位地址（几乎无限）。交易所目前主要用 IPv4，但 HFT 系统应支持双栈以兼容未来。"),
        ],
    },

    # ── 11.4 套接字接口 (111行, 丰富但无自测) ──
    {
        "filename": "section-11.4-套接字接口.md",
        "expand": None,
        "traps": [
            "**socket 返回 fd 但尚未连接** — 还需要 connect（客户端）或 bind+listen+accept（服务器）",
            "**accept 返回新 fd** — 监听 fd 继续接受新连接，不能用它收发数据",
            "**getaddrinfo 是协议无关的** — 同一代码兼容 IPv4/IPv6，替代废弃的 gethostbyname",
        ],
        "quiz": [
            ("Q1: socket() 返回什么？此时连接建立了吗？",
             "返回一个 fd（文件描述符），但此时尚未连接。客户端需 connect() 发起连接，服务器需 bind()+listen()+accept() 等待连接。"),
            ("Q2: accept() 返回的 fd 和监听 fd 有什么区别？",
             "accept() 返回一个新的 fd（connected fd），用于与客户端通信。监听 fd 继续接受新连接，不能用于收发数据。一个监听 fd 可对应多个 connected fd。"),
            ("Q3: 为什么推荐 getaddrinfo 而不是 gethostbyname？",
             "getaddrinfo 协议无关（自动处理 IPv4/IPv6）、线程安全（gethostbyname 返回静态缓冲区，非线程安全）、支持服务名解析。gethostbyname 已废弃。"),
            ("Q4: HFT 常用的 socket 选项有哪些？各解决什么问题？",
             "TCP_NODELAY：禁 Nagle 算法，降小包延迟。SO_REUSEPORT：多进程/线程同时收包。SO_BUSY_POLL：内核 busy poll，减少中断延迟。O_NONBLOCK+epoll：单线程管理多连接。"),
        ],
    },

    # ── 11.5 Web 服务器 (50行, 有内容) ──
    {
        "filename": "section-11.5-Web服务器.md",
        "expand": None,
        "traps": [
            "**HTTP 是无状态协议** — 每个请求独立，服务器不记得之前的请求；keep-alive 复用 TCP 连接但不保持应用状态",
            "**静态内容读磁盘，动态内容执行程序** — CGI 思想：URI 映射到可执行程序，程序输出即为 HTTP body",
            "**HFT 不用 HTTP 传行情** — HTTP 头部开销大、文本解析慢；但 admin API（风控面板、健康检查）可用 HTTP",
        ],
        "quiz": [
            ("Q1: HTTP 请求和响应的基本格式是什么？",
             "请求：方法(GET/POST) + URI + 版本 + 头部 + 空行(\\r\\n) + 可选body。响应：版本 + 状态码 + 头部 + 空行(\\r\\n) + body。每行以 \\r\\n 结尾。"),
            ("Q2: HTTP 的无状态是什么意思？keep-alive 改变了这一点吗？",
             "无状态 = 每个请求独立处理，服务器不记录之前请求的状态。keep-alive 复用 TCP 连接（减少握手开销），但应用层仍无状态。状态通过 Cookie/Session 在应用层维护。"),
            ("Q3: 静态内容和动态内容的服务方式有何不同？",
             "静态：直接读磁盘文件返回（HTML/图片）。动态：解析 URI，执行对应程序（CGI），程序输出作为 HTTP body 返回。Tiny 用 fork+execve 或函数指针表。"),
            ("Q4: HFT 为什么不用 HTTP 传输行情？什么场景用 HTTP？",
             "HTTP 头部开销大（百字节文本）、解析慢（文本协议）、无二进制支持。行情用 UDP multicast 或 TCP 自定义二进制协议。HTTP 用于 admin API（风控面板、健康检查、配置管理）。"),
        ],
    },

    # ── 11.6 综合TinyWebServer (29行, 有内容) ──
    {
        "filename": "section-11.6-综合TinyWebServer.md",
        "expand": None,
        "traps": [
            "**Tiny 是教学版本，每连接一个迭代** — 生产用线程池/epoll reactor，不能阻塞处理单个连接",
            "**rio_readlineb 逐行读** — 适合 HTTP 文本协议；二进制行情用固定长度 read，不用逐行",
            "**Tiny 的静态文件用 mmap** — 和 Ch9 mmap 联动：映射文件到 VA，直接 write 发送",
        ],
        "quiz": [
            ("Q1: Tiny Web Server 处理一个请求的完整流程？",
             "1) accept 连接；2) rio_readlineb 读请求行，解析 method/URI；3) 静态：stat+mmap 文件+rio_writen 响应头+body；动态：调用 serve_dynamic；4) close 连接。"),
            ("Q2: Tiny 的「每连接一个迭代」模式有什么问题？生产怎么解决？",
             "问题：处理一个连接时阻塞，其他连接等待。生产解决：1) 线程池（每连接一个线程）；2) epoll reactor（单线程非阻塞多路复用）；3) 协程（轻量级并发）。"),
            ("Q3: Tiny 用 rio_readlineb 读 HTTP 请求行，HFT 行情协议怎么读？",
             "HTTP 是文本行协议，用 rio_readlineb 逐行读。HFT 行情是定长二进制协议，用固定长度 read（如 recv(fd, buf, sizeof(msg), 0)），不逐行读。"),
            ("Q4: Tiny 的静态文件服务用 mmap，和 Ch9 学的 mmap 有什么关系？",
             "Tiny 用 mmap 将磁盘文件映射到 VA，然后直接 write/munmap 发送。利用了 Ch9 学的：mmap 创建 VA→文件映射，首次访问 page fault 装入数据，内核页缓存自动管理。"),
        ],
    },

    # ── 11.7 小结 (17行, 近空) ──
    {
        "filename": "section-11.7-小结.md",
        "expand": """### Ch11 全章要点

| 主题 | 核心概念 | HFT 关联 |
|------|----------|----------|
| §11.1 | C/S 模型、socket=fd | 行情客户端、订单网关 |
| §11.2 | 协议分层、LAN/WAN | 各层延迟优化 |
| §11.3 | IP+端口、DNS | 预解析 DNS，缓存 IP |
| §11.4 | socket API 全流程 | TCP_NODELAY、epoll、multicast |
| §11.5 | HTTP、静态/动态 | admin API 用 HTTP |
| §11.6 | Tiny Web Server | 迭代→并发演进 |

**一句话：** 网络编程 = socket API（socket→bind→listen→accept→read/write→close）+ 协议分层（应用/传输/网络/链路），HFT 用 TCP_NODELAY 降延迟、epoll 管多连接、UDP multicast 收行情。

""",
        "traps": [
            "**socket API 顺序不能乱** — 客户端：socket→connect→read/write；服务器：socket→bind→listen→accept→read/write",
            "**HFT 网络延迟优化是全栈的** — 不只应用层，链路层（NIC）、传输层（TCP 选项）都要管",
            "**Tiny Web Server 是教学版** — 生产用 epoll reactor + 线程池，不能阻塞迭代",
        ],
        "quiz": [
            ("Q1: 客户端和服务器的 socket API 调用顺序分别是什么？",
             "客户端：socket() → connect() → read/write() → close()。服务器：socket() → bind() → listen() → accept() → read/write() → close()。"),
            ("Q2: HFT 网络编程的三个关键优化是什么？",
             "1) TCP_NODELAY 禁 Nagle（降小包延迟）；2) epoll 单线程管多连接（避免线程切换）；3) UDP multicast 收行情（一对多，低延迟）。"),
            ("Q3: socket API 中哪些调用可能阻塞？HFT 如何处理？",
             "connect（TCP 握手）、accept（等待连接）、read（等待数据）都可能阻塞。HFT 用 O_NONBLOCK + epoll：所有 socket 设非阻塞，epoll 通知就绪事件，不阻塞等待。"),
            ("Q4: 从 Tiny Web Server 到生产级网络服务，需要哪些改进？",
             "1) 迭代→并发（线程池/epoll reactor）；2) 阻塞→非阻塞（O_NONBLOCK）；3) 单进程→多进程（SO_REUSEPORT）；4) 无监控→健康检查+限流+日志。"),
        ],
    },
]


def build_replacement(entry):
    parts = []
    if entry["expand"]:
        parts.append(entry["expand"])
    parts.append("### 常见陷阱\n")
    for i, trap in enumerate(entry["traps"], 1):
        parts.append(f"{i}. {trap}\n")
    parts.append("\n")
    parts.append("### 自测题\n\n")
    for q, a in entry["quiz"]:
        parts.append(f"<details>\n<summary>{q}</summary>\n\n{a}\n\n</details>\n\n")
    return "".join(parts)


def process_file(entry):
    filepath = os.path.join(NOTES_DIR, entry["filename"])
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    replacement = build_replacement(entry).rstrip()

    if SHELL_PATTERN in content:
        new_content = content.replace(SHELL_PATTERN, replacement)
        mode = "replace shell"
    else:
        # 11.4 没有 self-test，在导航行前插入
        nav_marker = "\n---\n\n← [本章导读](../README.md)"
        if nav_marker in content:
            new_content = content.replace(nav_marker, "\n" + replacement + nav_marker)
            mode = "insert before nav"
        else:
            print(f"  [SKIP] {entry['filename']} — 未找到匹配模式")
            return False

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"  [OK] {entry['filename']} ({mode})")
    return True


def main():
    print(f"Ch11 网络编程批量改造 — 共 {len(SECTIONS)} 个文件\n")
    success = 0
    skipped = 0
    for entry in SECTIONS:
        if process_file(entry):
            success += 1
        else:
            skipped += 1
    print(f"\n完成：{success} 成功，{skipped} 跳过")

    # 验证
    import glob
    files = glob.glob(os.path.join(NOTES_DIR, "*.md"))
    shell_remaining = 0
    details_count = 0
    for f in files:
        with open(f, "r", encoding="utf-8") as fh:
            c = fh.read()
            if "待口述补" in c:
                shell_remaining += 1
            details_count += c.count("<details>")
    print(f"\n验证：残留空壳 {shell_remaining} 个，<details> 标签 {details_count} 个")


if __name__ == "__main__":
    main()
