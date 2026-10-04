#!/bin/bash
# container-oneliners.sh — 容器域命令精选（02-ch15 §2/5/6）
set -u

cat <<'EOF'
===== nsproxy 命名空间链路（§2 块1/2，内核里认容器的方式） =====
$task = (struct task_struct *)curtask;
$pidns    = $task->nsproxy->pid_ns_for_children->ns.inum;   # PID ns 编号
$nodename = $task->nsproxy->uts_ns->name.nodename;          # UTS ns 主机名（=pod 名）

===== cgroup 维度采样与过滤（§6 块6） =====
# 按 cgroup ID 分解 99Hz 采样（哪个 cgroup 在吃 CPU）
bpftrace -e 'profile:hz:99 { @[cgroup_id] = count(); }'

# 只跟踪指定容器（cgroupid() 把 cgroup 路径变成可比较的 id）
bpftrace -e 'tracepoint:syscalls:sys_enter_openat
    /cgroup == cgroupid("/sys/fs/cgroup/container1")/
    { printf("%s\n", str(args->filename)); }'

===== overlayfs 容器文件视图（§5 块5 骨架） =====
PID=$(docker inspect -f '{{.State.Pid}}' <容器ID>)
# 拿到 nsid 后: bpftrace ./overlayfs.bt $NSID（书上 overlayfs.bt 按 ns 过滤）

===== 已落成脚本 =====
sudo bpftrace ../pidns-switch.bt   # 容器边界调度穿越
sudo bpftrace ../blkthrot.bt       # 块 I/O 节流按 cgroup

===== 配套 =====
06.6/ch11 的 cgroup-throttle-check.sh（CPU 节流三件套）
EOF
