#!/bin/bash
# ecosystem-cheatsheet.sh — BPF 生态工具速查（02-ch17 §2/4/5）
# 出自: notes/section-2-Vector与PerformanceCo-Pilot.md、
#       section-4-CloudflareeBPFPrometheus导出器.md、section-5-kubectl-trace.md
# 本机均未安装——这是"需要时怎么搭"的备忘
set -u

cat <<'EOF'
===== ① PCP + pmdabcc（BCC 指标接入监控体系，§2 块1/2） =====
cd /var/lib/pcp/pmdas/bcc && ./Install
# /var/lib/pcp/pmdas/bcc/bcc.conf:
#   modules = biolatency,sysfork,tcpperpid,runqlat,tcplife
#   [tcplife] cluster = 3, 可按 process/lport/dport 过滤
# 用途: 把 BCC 工具变成 PCP 指标源 → Vector/Grafana 长期趋势

===== ② Cloudflare ebpf_exporter（BPF → Prometheus，§4 块3/4） =====
git clone https://github.com/cloudflare/ebpf_exporter.git && make
sudo ./release/ebpf_exporter-*/ebpf_exporter --config.file=./examples/runqlat.yaml
# 监听 :9435/metrics；Prometheus 加 job（k8s 用 node role + relabel 10250→9435）
# 形态: yaml 里内嵌 BPF C，按 label 聚合导出——生产指标化的标准路径

===== ③ kubectl-trace（k8s 节点上跑 bpftrace，§5 块5/6） =====
git clone https://github.com/iovisor/kubectl-trace.git && make
kubectl trace run --node/<节点名> -f /usr/share/bpftrace/tools/vfsstat.bt
kubectl trace get                                  # 看任务状态
kubectl trace logs -f kubectl-trace-<id>           # 流式读输出
# 本质: 给 bpftrace 套了 k8s 调度——不用 ssh 上节点

===== 选型 =====
  单机即时排查     → bpftrace/BCC 直接上（本仓各章脚本）
  长期指标趋势     → ebpf_exporter → Prometheus
  已有 PCP 体系    → pmdabcc
  k8s 集群         → kubectl-trace（或更新的 inspektor-gadget）
EOF
