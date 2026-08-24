# P7 Part A — 主机侧 poll 循环（不是 DPDK）

笔记本 WSL **没有** VFIO/大页/PMD。这里用数组模拟两个口之间的 busy-poll 转发，只为看懂「热路径是一个死循环」。

真 DPDK：装 `libdpdk-dev` 后按 [../README.md](../README.md) 跑 testpmd。

```bash
make test
```
