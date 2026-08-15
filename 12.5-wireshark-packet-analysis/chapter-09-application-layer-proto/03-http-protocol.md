# 9.3 超文本传输协议（HTTP）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · TCP：[§8.1](../chapter-08-transport-layer-tcp-udp/02-tcp-connection-management.md)

**核心主旨**：基于 TCP 的请求/响应；GET/POST、状态码、压缩与重定向的抓包读法。

## 核心知识点

### 9.3.1 使用 HTTP 浏览

| 前提 | TCP 三次握手完成后；默认端口 **80** / **8080** |
|------|-----------------------------------------------|

**GET 请求典型字段**

| 字段 | 说明 |
|------|------|
| Request URI | 如 `/` |
| HTTP Version | `HTTP/1.1` |
| **User-Agent** | 浏览器标识 |
| Accept / Accept-Language | 内容与语言偏好 |
| **Cookie** | 会话状态 |

**响应**

| 项 | 说明 |
|----|------|
| 状态行 | 如 `HTTP/1.1 200 OK` |
| 体 | HTML 等，常分多 **TCP 段** 传输 |

| 易错 | 说明 |
|------|------|
| **Content-Encoding: gzip** | Wireshark 列表里 HTML 可能仍是压缩二进制 |
| 读明文 | **Follow TCP Stream** 或启用 HTTP 自动解压（Preferences → HTTP） |

**Wireshark**：`http` · `http.request.method == "GET"` · `http.response.code == 200`

---

### 9.3.2 POST 与重定向

| 方法 | 用途 |
|------|------|
| **POST** | 提交表单；URI 常为脚本路径如 `/wp-comments-post.php` |
| 体 | `application/x-www-form-urlencoded` 或 `multipart` |

**302 Found**

| 字段 | 说明 |
|------|------|
| 状态码 | **302** |
| **Location** | 浏览器跳转目标 URL |

**过滤器**：`http.response.code == 302` · `http.location`

> **拓展**：**HTTPS** = HTTP over TLS，见 TLS 握手（Server Hello、证书）；**HTTP/2** 多路复用单连接多流，协议树为 `HTTP2` 帧。

## 抓包/实操记录

| 实验 | 步骤 |
|------|------|
| 明文 HTTP | 访问 `http://neverssl.com` 或实验站点 → `http` |
| 看 GET | 展开 Hypertext Transfer Protocol → Request URI / User-Agent |
| gzip | 对比 Raw 与 Follow Stream 解码正文 |
| POST+302 | 提交表单抓包 → 找 POST 与紧随的 302 Location |

```bash
tshark -r cap.pcapng -Y "http.request" -T fields -e http.host -e http.request.uri -e http.user_agent
```

## 疑问与总结

- 现代站点多为 **HTTPS**；需 TLS 密钥或 key log 才能解 HTTP 内容。
- `http` 过滤器仅见已解码为 HTTP 的流；端口 443 默认显示 TLS。
