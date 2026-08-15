# 10.6 生气的开发者（FTP 自证清白）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：用 **STOR + Follow TCP Stream + MD5** 证明网络字节级无损，甩锅应用层。

## 核心知识点

### 故障现象

- 开发者 **FTP 上传 CSV** 到中心库后数据损坏；**归咎网络**。

### 抓包举证

| 步 | 操作 |
|----|------|
| 1 | `ftp.request.command == "STOR"` 定位上传控制信令 |
| 2 | 找到对应 **tcp.stream**（或数据连接端口） |
| 3 | 右键 → **Follow TCP Stream** → Save as **raw** → `extracted.csv` |
| 4 | 对 `extracted.csv` 与客户端源文件算 **MD5/SHA256** |

### 结论

| MD5 一致 | 含义 |
|----------|------|
| ✅ 相同 | 网络 **100% 字节级正确**；损坏在 **数据库/导入代码** |
| ❌ 不同 | 再查 FTP 模式（ASCII/二进制）、中间代理、重传是否真损坏载荷 |

> **拓展**：**NetworkMiner** 等可从 pcap **批量 Carving** FTP/HTTP 文件，提速取证。

## 抓包/实操记录

```bash
# 列 STOR
tshark -r cap.pcapng -Y 'ftp.request.command == "STOR"' -T fields -e tcp.stream -e ftp.arg

# 跟流后本地
certutil -hashfile extracted.csv MD5   # Windows
md5sum extracted.csv                   # Linux
```

| 注意 | FTP **主动/被动** 模式数据连接端口不同；跟错流会 hash 错 |
| 二进制 | 确认 **TYPE I**（图像）而非 TYPE A 改换换行 |

## 疑问与总结

- 网络工程师价值：**可重复的载荷证据**，而非口头「网络没问题」。
- 与 [§10.1](./01-lost-web-content.md) 对比：10.1 是网络真有问题；10.6 是网络无问题。
