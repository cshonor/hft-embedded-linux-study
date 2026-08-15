# 坑点清单

1. **XDP/TC 包解析漏边界检查** → 验证器直接拒载；每层头一个 if，别嫌啰嗦
2. **改了 IP 地址忘记重算校验和** → 对端静默丢包，ping 都不通，最难查的一类；改 ICMP type 同理要修 ICMP 校验和
3. **校验和折叠顺序**：先 16bit 累加全部字段，最后 `~((sum & 0xffff) + (sum >> 16))` 折叠，顺序错结果错
4. **XDP_TX 只能从进入的网卡发出**；跨网卡必须 `XDP_REDIRECT` + `bpf_redirect(ifindex, flags)`
5. **一个接口只能挂一个 XDP 程序**；需要多程序用 libxdp 或 TC（TC 原生支持链式多程序）
6. **netns 隔离**：XDP 程序加载进哪个网络命名空间，就只能看见那个 ns 的接口和流量；实验环境常用 `ip netns exec` + 指定 ns 加载
7. **uprobe 拿 SSL_read 明文必须 entry+ret 配对**：entry 存 buf 指针到 hash map，retprobe 按 SSL* 取出——单挂 entry 只能看空缓冲区
8. **Go <1.17 程序挂 uprobe 参数取不到**（栈传参）；静态链接无符号；库路径架构相关——uprobe 三坑先排查
9. **性能对比要公平**：XDP 丢包快是因为没建 sk_buff 没进协议栈，拿 XDP 和 iptables 比"每秒丢包数"差几个量级是正常的，不是测量错误
