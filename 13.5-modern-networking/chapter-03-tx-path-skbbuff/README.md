# Chapter 03: 发包路径与 sk_buff

> 来源：Bootlin（发包路径）+ kernel-docs（txrx + sock/sk_buff）+ LWN（sk_buff/xdp_buff）
> 对标：Rosen Ch3/5

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tx-path-bootlin](notes/01-tx-path-bootlin.md) | Bootlin：发包路径、qdisc → driver xmit → completion |
| 2 | [txrx](notes/02-txrx.md) | kernel-docs：发送/接收路径 API、ndo_start_xmit |
| 3 | [sock-sk-buff](notes/03-sock-sk-buff.md) | kernel-docs：sk_buff 结构、alloc/free、data layout |
| 4 | [sk-buff-xdp-buff](notes/04-sk-buff-xdp-buff.md) | LWN：sk_buff vs xdp_buff、XDP 绕过 sk_buff 分配 |

## HFT 关联

- **sk_buff 开销**：每个 sk_buff 分配约 200-300ns + 元数据初始化；XDP 用 xdp_buff 跳过 sk_buff
- **qdisc 延迟**：默认 fq_codel qdisc 引入排队延迟；HFT 发包应用 `pfifo_fast` 或直接 bypass
- **TSQ（TCP Small Queues）**：限制每连接发包队列深度，防止 bufferbloat；HFT 需要调大 `txqueue_len`
- **零拷贝发送**：MSG_ZEROCOPY 避免 sk_buff 数据拷贝，见 ch13

## 交叉引用

- `13.5-modern-networking/chapter-05-xdp-architecture/`：xdp_buff 绕过 sk_buff
- `13.5-modern-networking/chapter-13-zerocopy-highperf/`：MSG_ZEROCOPY 零拷贝发送
