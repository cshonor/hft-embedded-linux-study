# 3. BPF 工具：套接字层（10.3.1–10.3.8）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p448–470）

覆盖 8 个工具：sockstat、sofamily、soprotocol、soconnect、soaccept、socketio、socksize、sormem。

## 3.1 sockstat —— 套接字系统调用速率

- 统计每秒 `accept/connect/bind/socket` 系统调用（syscall 跟踪点）+ kprobe `sock_recvmsg/sock_sendmsg`。
- 案例：java 进程 **10547 次 read/秒**——先定位"谁在疯狂收发"，再决定下钻方向。

## 3.2 sofamily —— 按协议族分类

- `AF_UNIX / AF_INET / AF_INET6` 按 comm 统计。
- 实现要点：`sockaddr` 哈希表**入口存、出口读**（bind/connect 时把 family 存入以 sock 为键的表，后续事件查表分类）。

## 3.3 soprotocol —— 按 TCP/UDP 协议分类

- 读 `sk->sk_prot->name`（"TCP"/"UDP"）最便捷但**属于不稳定结构体路径**，跨内核可能变。
- 稳定替代：LSM `security_socket_*` 函数族（如 security_socket_recvmsg）。

## 3.4 soconnect —— 主动连接事件表

```
PID   PROCESS  FAM  ADDRESS         PORT  LAT(ms)  RESULT
```

- `@err2str` 错误映射表把 errno 翻成字符串（ECONNREFUSED/EHOSTUNREACH…）。
- **非阻塞 connect 返回 EINPROGRESS** 属正常中间态，最终成败要看后续。

## 3.5 soaccept —— 被动接受事件表

- 仅显示**远端地址**（accept 返回的客户端地址）；需要本地+远端双方视角用 `tcpaccept`（10.3.12）。

## 3.6 socketio —— 收发事件频率与延迟

- 跟踪点选 `sock_recvmsg/sock_sendmsg`：**所有套接字路径的交集**（无论 read/write/sendto/recvmsg 最终都经过）。
- 陷阱：`socket_file_ops` 的 `read_iter/write_iter` 路径会**遗漏**——不能只跟 VFS 侧。

## 3.7 socksize —— 收发字节直方图

```bash
bpftrace: hist(args->retval)   # 以返回值为字节数
@filter: retval < 0x7fffffff    # 过滤负值（errno）
```

- 变体：stats() 输出 count/average/total 三种聚合。

## 3.8 sormem —— 接收缓冲区水位

- `rmem_alloc` vs `rmem_limit` 直方图——逼近上限即接收丢包前兆。
- 跟踪点：`sock:sock_rcvqueue_full`（已满丢包）、`sock:sock_exceed_buf_limit`（超限告警）。
- 调优：sysctl `tcp_rmem` + `tcp_moderate_rcvbuf`（自动调节接收窗）。

## HFT 关联

- 行情接收进程出现丢包 → 先 `sormem` 看缓冲水位，再 `sofamily` 确认是否走了意外协议族（如回退到 IPv4）。
- 订单网关连接风暴 → `sockstat` 看系统调用速率 + `soconnect` 看失败错误码分布。

<details>
<summary>自测题</summary>

1. 为什么 sock_recvmsg/sendmsg 是 socketio 的最佳跟踪点？
   <details><summary>答案</summary>它们是**所有套接字 I/O 路径的汇聚点**：read/write/readv/sendto/sendmsg/recvmsg 无论从哪个系统调用进来，最终都走 sock_recvmsg/sock_sendmsg。在这里打一个点覆盖全部路径；反过来只跟踪 VFS 侧（socket_file_ops 的 read_iter/write_iter）会漏掉直走 sendmsg 族的应用。</details>

2. soconnect 中 LAT 与 RESULT 如何处理非阻塞 EINPROGRESS？
   <details><summary>答案</summary>EINPROGRESS 只是"SYN 已发出"的中间态，不算完成也不算失败——LAT 的终点要等到 poll/select 报告可写（握手完成），RESULT 记录的是最终的 errno（成功为 0）。把 EINPROGRESS 当失败统计会把所有非阻塞 connect 全算成错误。</details>

3. sormem 对应哪两个 sock 跟踪点？配套 sysctl 是什么？
   <details><summary>答案</summary>`sock:sock_rcvqueue_full`（接收队列满、开始丢包）与 `sock:sock_exceed_buf_limit`（超限告警）。调优 sysctl：`tcp_rmem`（接收缓冲 min/default/max）+ `tcp_moderate_rcvbuf`（自动调节接收窗开关）。</details>

4. soprotocol 直接读 sk_prot->name 的风险及稳定替代方案？
   <details><summary>答案</summary>sk->sk_prot->name 是**不稳定结构体路径**——内核结构体布局随版本变，直接解引用跨内核会读错。稳定替代：LSM `security_socket_*` 函数族（内核导出的稳定 hook 点）。</details>
</details>
