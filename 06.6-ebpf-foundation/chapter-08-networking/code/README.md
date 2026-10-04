# ch08 code · 网络三例（XDP / TC / uprobe-SSL）

> 《Learning eBPF》第 8 章示例补全为可编译程序。
> 笔记中 XDP 负载均衡片段（8.1 块3，改 MAC/IP/校验和 + XDP_TX）为教学片段，并入 xdp 文件注释。
> XDP/DPDK/AF_XDP 的生产对照见 [13-dpdk/02-Advanced-Book](../../../13-dpdk/02-Advanced-Book/notes/note-XDP与DPDK对照.md)。

## 程序

| 文件 | 出处 | 内容 | 验证（clang 18.1 -target bpf） |
|------|------|------|------|
| [xdp-proto-filter.bpf.c](./xdp-proto-filter.bpf.c) | 8.1 块1/2 补全 | XDP 边界检查范式 + 按 IP 协议计数（HASH map），改 DROP 即防火墙雏形 | 编译 ✓（无需 vmlinux.h） |
| [tc-pingpong.bpf.c](./tc-pingpong.bpf.c) | 8.1 块4 补全 | TC ICMP 应答器：换 MAC/IP、type 8→0、clone_redirect 原口发回 | 编译 ✓（自带 icmphdr，见下） |
| [ssl-sniff.bpf.c](./ssl-sniff.bpf.c) | 8.2 块5 补全 | uprobe 抓 SSL 明文：write entry 直读、read 用 "SSL*→buf" map 配对 entry/exit | 编译 ✓（需 vmlinux.h） |

## 加载（全部需 root）

```bash
# XDP（lo 用 xdpgeneric）
sudo ip link set dev lo xdpgeneric obj .build/xdp-proto-filter.o sec xdp
sudo bpftool map dump name proto_cnt        # 看协议计数
sudo ip link set dev lo xdpgeneric off

# TC pingpong
sudo tc qdisc add dev <nic> clsact
sudo tc filter add dev <nic> ingress bpf da obj .build/tc-pingpong.o sec classifier
ping <nic 的 IP>                             # 回包是 BPF 伪造的
sudo tc qdisc del dev <nic> clsact

# ssl-sniff 需配套用户态 loader（libssl 路径解析 + perf buffer），形态同 ch05
```

## 编译踩坑实录

- `linux/icmp.h` 在 `-target bpf` 下拉入 glibc 桩头（`gnu/stubs-32.h` 缺失）→
  tc 程序自带 `icmphdr` 定义（字段与 uapi 一致）
- `IPPROTO_ICMP` 在 `linux/in.h` 而非 `linux/ip.h` → 自带 `#define IPPROTO_ICMP 1`
