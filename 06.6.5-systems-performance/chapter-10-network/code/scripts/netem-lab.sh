#!/bin/bash
# netem-lab.sh — iperf3 吞吐 + tc netem 损伤注入（ch10 §10.7-10.8 块4）
# 用法: ./netem-lab.sh bench <server_ip>   —— iperf3 4 流聚合吞吐
#       ./netem-lab.sh impair [网卡]        —— 注入 2ms 延迟+0.1% 丢包+乱序（测试环境！）
#       ./netem-lab.sh heal [网卡]          —— 撤销损伤
set -u

case "${1:-}" in
bench)
    command -v iperf3 >/dev/null || { echo "未安装: sudo apt install iperf3"; exit 1; }
    iperf3 -c "${2:?缺 server ip}" -t 30 -P 4
    ;;
impair)
    NIC="${2:-eth0}"
    echo "⚠ 测试环境专用！向 $NIC 注入: 2ms 延迟 + 0.1% 丢包 + 0.5% 乱序"
    echo "sudo tc qdisc add dev $NIC root netem delay 2ms loss 0.1% reorder 0.5%"
    echo "撤销: $0 heal $NIC"
    ;;
heal)
    NIC="${2:-eth0}"
    echo "sudo tc qdisc del dev $NIC root"
    ;;
*)
    echo "用法: $0 bench <server_ip> | impair [网卡] | heal [网卡]"; exit 1;;
esac
