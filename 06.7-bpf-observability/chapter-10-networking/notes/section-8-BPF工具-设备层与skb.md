# 8. BPF 工具：设备层与 skb（10.3.26–10.3.30）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p515–525）

覆盖 5 个工具：netsize、nettxlat、skbdrop、skblife、ieee80211scan。

## 8.1 netsize —— 物理层收发包尺寸直方图

- 四个跟踪点：`net:netif_receive_skb`（收）、`net:net_dev_queue`（发排队）、`net:napi_gro_receive_entry`（GRO 合并）、`net:net_dev_xmit`（发送完成）。
- 用途：**NIC 与内核栈看到的大小对比**（GRO/TSO 卸载前后差异），判断巨型帧/合并是否生效。
- 案例：iptraf-ng 观测时 **90% CPU** vs netsize **0%**——libpcap 全量拷贝 vs BPF 聚合的差距。

## 8.2 nettxlat —— 发送延迟（排队→发出）

- `net_dev_start_xmit` → `skb:consume_skb`（释放即发送完成路径）。
- 陷阱：**net_dev_queue 删除时间戳防复用**（qdisc 重入队会再次经过该点，需清键防误配对）。
- 变体 nettxlat-dev 按设备统计，但需读不稳定结构体。

## 8.3 skbdrop —— 丢包点定位

```
KSTACK（8 帧）+ 自动 nstat 计数
```

- 跟踪 `skb:kfree_skb`（非正常路径释放，即丢包）+ 内核栈 8 帧 = **丢在哪个函数**。
- 需 `bpftrace --unsafe`（内部调用 system() 跑 nstat）。
- 案例：TcpExtTCPDeferAcceptDrop——defer accept 期间数据未到即被计为丢。

## 8.4 skblife —— skb 生命周期总时长

- kprobe `kmem_cache_alloc`，检查缓存名是 `skbuff_head_cache` / `fclone_cache` 才打点；释放（kfree）算寿命。
- 局限：**GSO/GRO/tcp_try_coalesce 会合并 skb**——合并后原 skb 提前释放，寿命偏短。

## 8.5 ieee80211scan —— WiFi 扫描阻塞

- wpa_supplicant 扫描阻塞 **3205ms** 案例：单全局 `@start` 假设无并行扫描（WiFi 单卡场景成立）。
- 教训：单例假设要在工具注释里写明适用条件。

## HFT 关联

- 万兆行情链路偶发丢包：skbdrop 栈帧直接指向丢包函数（缓冲满/校验失败/路由 miss），配合 `ethtool -S` 队列计数交叉验证。
- netsize 验证网卡 RSS 队列分布下的包尺寸特征，GRO 合并过大可能导致 DPU/软中断单核热点。

<details>
<summary>自测题</summary>

1. netsize 用哪四个跟踪点？为什么 CPU 占用远低于 iptraf-ng？
   <details><summary>答案</summary>`net:netif_receive_skb`（收）、`net:net_dev_queue`（发排队）、`net:napi_gro_receive_entry`（GRO 合并）、`net:net_dev_xmit`（发送完成）。iptraf-ng 走 libpcap——每个包完整拷贝到用户态再过滤渲染（书例 90% CPU）；netsize 在内核态只做直方图计数（0%）——聚合与逐包搬运的开销差就是这两个数量级。</details>

2. nettxlat 为什么要删除时间戳？
   <details><summary>答案</summary>skb 可能被 qdisc **重入队**——再次经过 net_dev_queue 打点。不清掉旧时间戳的话，重入队的 skb 会拿第一次的时间戳配第二次的出队，产生巨大假延迟（ch05 "入口未记录"陷阱的镜像版：这里是"旧账未清"）。</details>

3. skbdrop 的 kfree_skb 为什么能代表"丢包"？--unsafe 从何而来？
   <details><summary>答案</summary>skb 的正常善终是 consume_skb（被消费掉）；kfree_skb 是非正常路径释放（校验失败、缓冲满、协议栈放弃）——它就是内核丢包的汇聚点，此时的内核栈指向"谁丢的"。--unsafe 是因为工具内部调 system() 跑 nstat 做计数对账——system() 属于不安全内建，需要显式解锁。</details>

4. skblife 的合并问题如何影响测量？
   <details><summary>答案</summary>GSO/GRO/tcp_try_coalesce 会把多个 skb 合并成一个——原 skb 在合并点被提前 kfree，测到的"寿命"其实是"合并前寿命"，系统性偏短。读数字时记住这是下界视角，不是端到端真相。</details>
</details>
