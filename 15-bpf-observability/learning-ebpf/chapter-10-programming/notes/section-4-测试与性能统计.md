# 测试与性能统计

- **BPF_PROG_RUN**：用户态直接运行 eBPF 程序做测试（目前主要支持网络类程序类型）
- 运行时统计：

```sh
sysctl -w kernel.bpf_stats_enabled=1
bpftool prog list     # 多出 run_time_ns / run_cnt 字段
# 例：run_time_ns 316876 run_cnt 4 → 4 次执行共 ~300μs
```
