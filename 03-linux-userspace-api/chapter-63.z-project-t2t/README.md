# 63-t2t — tick-to-trade 三进程管线（毕业项目，P3）

**触发**：学完 Ch56–63（套接字、服务器设计、epoll）笔记后开写。
**复用**：chapter-55.z-project-shmring 的环形缓冲与延迟测量代码。

## 架构

```
行情接入(UDP 组播, recvmmsg) --SHM ring--> 策略引擎(epoll+timerfd) --SHM ring--> 订单网关(TCP)
```

三个进程、两条 SHM 环；观测面用 bpftrace 从内核侧验证应用侧数字。

## 需求清单

- [ ] feeder：UDP 组播收行情包（自造简单二进制协议），`recvmmsg` 批量收
- [ ] strategy：epoll 等 SHM 数据 + timerfd 做风控超时，简单均值回复策略
- [ ] gateway：TCP 连接下游模拟撮合，订单带原始 tick 时间戳
- [ ] 全链路延迟 = 网关发出时刻 − feeder 收包时刻，输出 P50/P99
- [ ] SCHED_FIFO + CPU 亲和性把三个进程钉到不同核
- [ ] bpftrace 脚本：tracepoint 上测 recv→send 的内核侧耗时，与用户态数字对比

## 验收标准

- Pi 5 上端到端 tick→order 延迟有完整分布表
- 用户态与 bpftrace 两个来源的数字能互相印证（差异 < 2 倍并解释原因）
- README 写清：每一段延迟占多少、瓶颈在哪、下一步优化方向

## HFT 关联

这就是单机版的真实 HFT 链路拓扑；做完它，TLPI 里与低延迟相关的章节（4–5、13、23、35、49–55、56–63）全部经过实战。
