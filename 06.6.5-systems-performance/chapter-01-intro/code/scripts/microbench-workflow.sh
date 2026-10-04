#!/bin/bash
# microbench-workflow.sh — 微观基准标准流程（隔离核 + 预热 + rdtsc）
# 出自: notes/section-1.8-实验与微观宏观基准.md 块7
# 用法: ./microbench-workflow.sh <你的microbench程序> [核号]
# 配套: 微观快、宏观没快 → 查 resource 争用（见 1.4 双视角），不是继续死抠模块
set -u

BENCH="${1:?用法: $0 <microbench程序> [核号(默认2)]}"
CORE="${2:-2}"

echo "===== ① 检查核 $CORE 是否被 isolcpus 隔离 ====="
ISOL=$(cat /sys/devices/system/cpu/isolated 2>/dev/null || echo "无")
echo "isolated CPUs: ${ISOL:-无}"
case ",$ISOL," in
    *",$CORE,"*|*"$CORE-"*) echo "核 $CORE 已隔离 ✓" ;;
    *) echo "⚠ 核 $CORE 不在隔离列表里——结果会受调度干扰（boot 参数 isolcpus= 设置）" ;;
esac

echo
echo "===== ② taskset 绑核运行 ====="
echo "taskset -c $CORE $BENCH"
taskset -c "$CORE" "$BENCH"

cat <<'EOF'

===== ③ 程序内规范（被测程序应做到） =====
  mlock 输入 buffer → warm-up 1e6 次 → rdtsc 测 1e7 次 → 报 ns/op
  （参考实现: 13-dpdk/01-Intro-Book/code/mcast-minimal/src/hist.h 的分位直方图）

===== ④ 宏观回放验证 =====
  微观提速后，同 workload 全链路回放，确认 Mean/P999/Max 真降才算数。
EOF
