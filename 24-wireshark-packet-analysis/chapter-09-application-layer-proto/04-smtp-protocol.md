# 9.4 简单邮件传输协议（SMTP）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · DNS MX：[§9.2](./02-dns-protocol.md)

**核心主旨**：MUA/MTA、SMTP 命令序列、STARTTLS 与 multipart/base64 附件。

## 核心知识点

### 9.4.1 收发邮件与角色

| 角色 | 说明 |
|------|------|
| **MUA** | 用户邮件客户端（Outlook、Thunderbird） |
| **MTA** | 邮件传输代理（Postfix、Exchange 等） |
| 跨域 | 本地 MTA 查 **MX** → SMTP 投递远程 MTA |

**端口**：SMTP **25**（MTA-MTA）、提交 **587**（常 STARTTLS）；**465** 隐式 TLS。

---

### 9.4.2 跟踪一封邮件（SMTP 对话）

TCP 建立后 **文本命令/响应**（Follow TCP Stream 最直观）：

| 步 | 方向 | 示例 |
|----|------|------|
| 1 | S→C | `220` 服务就绪（含 ESMTP） |
| 2 | C→S | **EHLO** / HELO |
| 3 | S→C | `250` + 扩展（SIZE, **STARTTLS**, …） |
| 4 | C→S | `MAIL FROM:<sender>` |
| 5 | C→S | `RCPT TO:<rcpt>` |
| 6 | C→S | **DATA** |
| 7 | C→S | 正文 + 行 **`.`** 结束（`<CR><LF>.<CR><LF>`） |
| 8 | S→C | `250` 已接收 |
| 9 | C→S | **QUIT** |

**MTA → MTA**：与对方 25 端口重复上述过程。

**收取邮件（对比）**

| 协议 | 说明 |
|------|------|
| POP3 / IMAP | 收件；常在明文连接后发 **STARTTLS** 升级 TLS |
| 抓包 | `imap` · `pop`；TLS 后为加密载荷 |

**Wireshark**：`smtp` 或 Follow Stream 读 ASCII 命令。

---

### 9.4.3 附件与 Base64

| 机制 | 说明 |
|------|------|
| Content-Type | `multipart/mixed` |
| **boundary** | 分隔正文与附件部分 |
| 附件编码 | **Content-Transfer-Encoding: base64** |

| 高危误区 | **Base64 不是加密**；嗅探即可还原附件 |
| 防护 | **STARTTLS** / SMTPS、端到端 PGP 等 |

## 抓包/实操记录

| 实验 | 操作 |
|------|------|
| 实验环境 | 本地 Mailhog / lab SMTP，抓 25/587 |
| 读对话 | Follow TCP Stream，标 220/EHLO/MAIL/DATA/QUIT |
| MX | `nslookup -type=mx domain` 后对比 SMTP 连接目标 IP |

```bash
tshark -r cap.pcapng -Y "smtp" -T fields -e frame.number -e smtp.req.command
```

## 疑问与总结

- 公网明文 SMTP 已少见；多数为 TLS 包裹，需解密或看握手前 EHLO。
- 钓鱼分析：核对 `MAIL FROM` 与 `Received` 链、SPF/DKIM 在邮件头（MIME）内。
