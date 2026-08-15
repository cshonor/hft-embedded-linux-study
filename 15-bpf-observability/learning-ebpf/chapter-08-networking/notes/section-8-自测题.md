# 自测题

1. XDP 的五种返回码各是什么？`XDP_TX` 和 `XDP_REDIRECT` 的本质区别？
2. 为什么 `xdp_md` 里没有 len 字段？包长怎么算？
3. 写出 XDP 解析到 TCP 头为止需要的三次边界检查。
4. XDP 负载均衡改完目的 IP 后必须做什么？为什么？
5. TC 的上下文和 XDP 有什么不同？TC 哪两个方向可挂程序？
6. `bpf_clone_redirect` 和 `bpf_redirect` 的区别？ping-pong 示例最后为什么返回 TC_ACT_SHOT？
7. 钩 SSL_read 看明文为什么必须 uprobe+uretprobe 配对？用什么做两个探针间的关联 key？
8. 2 万 Service 时 iptables 全量重写要 5 小时，eBPF 方案为什么是秒级？
9. Cilium 在一台节点上分别在哪些层挂了程序、各负责什么？
10. Cilium NetworkPolicy 比 K8s 原生策略强在哪三点？
