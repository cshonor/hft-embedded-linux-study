# 01 · CDN Edge（轻量边缘缓存）

> 索引：[08 实战项目](../README.md)

## 是什么

单节点 **HTTP 静态资源边缘缓存**：`GET`/`HEAD`、**内存 LRU**、未命中时从 **origin 目录** 读取并回填。非全球 DNS 调度，诚实定位为 **练手级 CDN 边缘**。

## 架构

```text
Client ──GET /path──► axum (cdn-edge)
                         ├─ 内存 LRU 命中 → 200 + Cache-Control
                         └─ 未命中 → 读 origin 目录 → 写入缓存 → 200
```

## 技术栈

| 项 | 选型 |
|----|------|
| 运行时 | tokio |
| HTTP | axum |
| 缓存 | 进程内 LRU（`bytes`） |
| 后续 | 磁盘二级缓存 · Range · ETag · purge API |

## 运行

```bash
mkdir -p 08-Practice-Projects/01-cdn-edge/origin
echo hello > 08-Practice-Projects/01-cdn-edge/origin/test.txt
cargo run -p cdn-edge -- --port 8080 --origin ./08-Practice-Projects/01-cdn-edge/origin
curl http://127.0.0.1:8080/test.txt
```

## Roadmap

- [x] MVP：origin 回源 + 内存缓存
- [ ] `ETag` / `If-None-Match` → 304
- [ ] `Range` 请求
- [ ] `POST /purge` 管理接口
- [ ] 指标：命中率、延迟直方图

## 设计取舍

1. **单进程内存缓存** — 先验证 async I/O 与缓存语义，再考虑磁盘。
2. **origin 为本地目录** — 避免 MVP 依赖 S3 SDK。
3. **不做 TLS 终止** — 前置 nginx/Caddy 即可。

## 对外描述（GitHub）

> Rust 实现的轻量静态资源 CDN 边缘节点：异步 HTTP、内存 LRU、origin 回源。
