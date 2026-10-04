#!/bin/bash
# hypervisor-detect.sh — 虚拟化环境检测与观测入口（02-ch16 §2/3 块1/2/7）
# 出自: notes/section-2-传统工具.md、section-3-BPF工具-Xen超级调用.md
set -u

echo "===== ① 是否在虚拟机里（什么 Hypervisor） ====="
dmesg 2>/dev/null | grep -i hypervisor || sudo dmesg 2>/dev/null | grep -i hypervisor || \
    echo "（无 Hypervisor 记录 = 裸金属，本机即此情况）"
systemd-detect-virt 2>/dev/null || true

echo
echo "===== ② 裸金属 → 本章工具用不上，去 13-dpdk ch10-13（为什么 HFT 选裸金属） ====="

cat <<'EOF'

===== ③ 在 KVM 宿主机上：perf kvm stat（块1） =====
perf kvm stat live
# 读数（VM-EXIT 原因分布）：
#   HLT 占比高          = guest 常 halt（正常，idle VM）
#   EPT_MISCONFIG       = EPT 配置事故
#   MSR_WRITE 频繁      = guest 在写 MSR（特殊设备访问）
#   IO_INSTRUCTION      = 半虚拟化前的慢路径（应被 virtio 消灭）

===== ④ 在 Xen 上（块3-7）：xen: tracepoint 一族 =====
funccount-bpfcc 't:xen:*'                          # Xen 事件覆盖（PV 才非零；HVM 全 0）
argdist-bpfcc -t 't:xen:xen_mc_flush():int:args->mcidx' -i 1   # 批量数分布
stackcount-bpfcc -t 'xen:xen_mc_issue'             # 谁触发的（fork/缺页/调度）
funclatency-bpfcc xen_mc_flush                     # 耗时直方图

# 块7 的重要教训：Xen HVM 下 xen_mc* 全为 0 ——
# 工具没坏，是架构不同（HVM 用硬件虚拟化，不走 paravirt 超级调用）。
# 先 dmesg 确认 PV 还是 HVM，再决定用哪套工具。
EOF
