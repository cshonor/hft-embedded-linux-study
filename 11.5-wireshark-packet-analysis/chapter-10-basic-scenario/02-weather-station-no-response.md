# 10.2 无响应的气象服务

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · HTTP：[§9.3](../chapter-09-application-layer-proto/03-http-protocol.md)

**核心主旨**：IoT/第三方无日志时，用抓包区分**链路故障**与**应用层认证失败**；警惕 HTTP 明文传参。

## 核心知识点

### 故障现象

- 气象站接收器**停止向云端同步**；设备 UI **无有效错误码**。

### 抓包分析

| 步 | 动作 | 结果 |
|----|------|------|
| 1 | 锁定目标 IP；**WHOIS** | 归属正常组织 |
| 2 | **Follow TCP Stream** | **HTTP GET**，参数在 **URL 查询串** 明文传气象数据 + 认证 |
| 3 | 响应行 | `HTTP/1.0 200 OK` |
| 4 | 响应体 | 文本 **`INVALIDPASSWORD`** |

### 结论

| 层级 | 判定 |
|------|------|
| 网络 | **通**（有 TCP、有 HTTP 200） |
| 应用 | **密码被重置为 0** 等配置错误 → 认证失败 |

### 安全警示（高危）

| 风险 | 说明 |
|------|------|
| 密码在 URL | 如 `&PASSWORD=00000000` |
| 明文 HTTP | 链路上任何人可 **Follow Stream** 直接读到 |

**过滤器**：`http.request.method == "GET"` · `http.host` 含云端域名 · `http contains "INVALID"`

> **拓展**：HTTPS 抓包见 TLS 握手；证书错误常表现为 Alert，而非 200+业务错误串。

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| 区分断网 vs 应用错 | 有 **200** + 业务错误正文 → 查配置 |
| 证明明文泄露 | 导出 Follow Stream 给安全评审 |

## 疑问与总结

- **200 OK ≠ 业务成功**；务必读 **Body** 或 JSON 内 `error` 字段。
- IoT 排障优先：**抓包 > 猜设备日志**。
