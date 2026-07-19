## 11.6 综合：Tiny Web Server

> **Ch11 §11.6** · [章导读](../README.md) · 上节 [§11.5 ←](./section-11.5-Web服务器.md) · 下节 [§11.7 →](./section-11.7-小结.md)

---

流程摘要：

1. `accept` 连接
2. **读请求行** — `rio_readlineb` 解析 method / URI
3. **静态** — `stat` + `mmap` 文件 + `rio_writen` 响应头+body
4. **动态** — 调用 `serve_dynamic` 等
5. `close`

**与 HFT 类比：**

- **HTTP 解析** ≈ 任意 **文本行协议** admin API（风控面板、健康检查）
- **定长/二进制行情** 不用 HTTP — 但 **读请求头 + 路由** 模式类似
- Tiny 的 **每连接一个迭代** — 生产用 **线程池 / epoll reactor**

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§11.5 ←](./section-11.5-Web服务器.md) · [本章导读](../README.md) · [§11.7 →](./section-11.7-小结.md)
