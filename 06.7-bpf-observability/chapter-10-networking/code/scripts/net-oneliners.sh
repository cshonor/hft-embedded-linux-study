#!/bin/bash
# net-oneliners.sh — 网络域命令精选（02-ch10 §2/9/11）
set -u

cat <<'EOF'
===== 传统工具（§2 块5-9） =====
ss -tiepm                     # 全量列：内部栈、扩展、进程、内存
ip -s link                    # RX errors/dropped/overrun；TX carrier
nstat -S                      # -S = 不重置计数器（snapshot）
sar -n SOCK,TCP,ETCP,DEV 1
ethtool -S eth0               # 驱动私有统计
ethtool -k eth0               # TSO/GSO/GRO/校验卸载开关
# ethtool -K eth0 tso off     # 运行时调节（需 root）

===== BCC 成品（§4/11 块4/11） =====
tcpconnect-bpfcc -t           # 主动连接含时间戳（-P 1313 过滤端口）
tcplife-bpfcc                 # 连接画像（时长/吞吐/归属）
tcpretrans-bpfcc              # 重传/丢包事件

===== bpftrace 单行（§9 块12-16） =====
# connect 失败 + 用户栈（定位哪段代码在连）
bpftrace -e 'kretprobe:tcp_v4_connect /retval < 0/ { @[ustack(5)] = count(); }'
# TCP 发送字节数直方图
bpftrace -e 'kr:tcp_sendmsg { @[comm] = hist(retval); }'
# UDP 收发频率按进程
bpftrace -e 'k:udp_sendmsg, k:udp_recvmsg { @[comm, probe] = count(); }'
# 发包全链路栈（write→VFS→socket→TCP→IP→设备）
bpftrace -e 't:net:net_dev_xmit { kstack(15); exit(); }'
# 收包中断上下文路径
bpftrace -e 't:net:netif_receive_skb { @[kstack(10)] = count(); }'
# 驱动事件（换你的驱动名）
bpftrace -e 't:ixgbevf:ixgbevf_* { @[probe] = count(); }'
EOF
