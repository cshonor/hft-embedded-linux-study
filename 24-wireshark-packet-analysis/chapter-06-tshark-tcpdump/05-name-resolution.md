# 6.5 名称解析

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · GUI：[§5.3](../chapter-05-advanced-feature/03-name-resolution.md)

**核心主旨**：CLI 下默认 DNS/MAC 解析会卡顿；排障大数据集应关闭或精确控制。

## 核心知识点

| 痛点 | 后台大量 DNS/MAC 查询 → **严重变慢** |

### TShark

| 参数 | 说明 |
|------|------|
| **`-n`** | **禁用全部**名称解析（最常用，提速） |
| **`-N <flags>`** | 细粒度开启：`m` MAC · `n` 网络地址 · `t` 传输层端口 · `N` 外部 DNS 等 |

```bash
tshark -ni 1 -w out.pcapng    # -n 常写作 -ni
tshark -r out.pcapng -n
```

### tcpdump

| 参数 | 说明 |
|------|------|
| **`-n`** | 不做主机名解析 |
| **`-nn`** | 主机名 + **端口服务名** 都不解析（**实战推荐**：纯 IP + 数字端口） |

```bash
sudo tcpdump -nni eth0 -w out.pcap
```

## 抓包/实操记录

| 场景 | 建议 |
|------|------|
| 生产抓包 | 始终 `-n` / `-nn` |
| 恶意 pcap 离线 | TShark `-n`，禁 `-N` 含外部 DNS |
| 需要可读性 | 事后 GUI 开 hosts，或 `-N` 仅开 `t` |

## 疑问与总结

- 与 GUI「关闭 external resolver」同一原则。
- 抓包时用 `-nn`，分析报表时再对少量 IP 做 whois/hosts。
