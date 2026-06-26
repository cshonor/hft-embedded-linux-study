# 7. 底层：qdisc / skb / 驱动

### `qdisc-*` 家族

针对 **fq、codel、cbq** 等排队规则测量 **包排队延迟**。

**场景：** 出口 bufferbloat、云主机 qdisc 配置不当。

### `netsize`

**GSO/GRO 前后** 设备层 send/recv **包大小直方图**。

### `nettxlat`

**网卡驱动 TX 队列** 延迟 — 包进 ring → 硬件发完。

**HFT：** 区分 **软件栈慢** vs **NIC 发送队列拥塞**（与 `ethtool -S` 配合）。

### `skbdrop`

`sk_buff` **异常丢弃** + **内核栈** — 丢包元凶。

```bash
sudo skbdrop-bpfcc
```

**极 valuable：** `ip -s` 见 drop 但不知原因 → `skbdrop` 给 **函数栈**。

### `skblife`

`sk_buff` 从分配到释放的 **生命周期耗时** — 包在栈里「呆太久」。

### `ieee80211scan`

WiFi 802.11 扫描耗时 — 数据中心 HFT 少见，笔记本调试可用。

---
