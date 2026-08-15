# HFT 关联

- **低延迟监控工具就该单一二进制**：cilium/ebpf + bpf2go 或 libbpf 骨架产出无依赖单文件，push 到交易机零安装成本、可审计（与第 5 章 HFT 论点呼应）
- **opensnoop 式 entry/exit 配对直接可抄**：交易程序挂单路径计时（entry 存时间戳，exit 取出算差值，hist map 出分布）就是同一模式
- **bpf_stats_enabled 是免费的性能回归探针**：交易机常规 eBPF 程序的 run_time_ns/run_cnt 定期采集，可发现验证器/JIT 层面的异常开销
- **bpftrace 适合运维快速排查**（"这台交易机现在谁在疯狂 open 配置文件？"一行搞定），生产常驻 agent 用 CO-RE 栈
