#!/bin/bash
# hwlat-detect.sh — 硬件延迟检测（SMI/固件停顿，ch14 §14.9 块17）
# 用途: 排除"内核以上都查了但还有周期性停顿"的最后一环——固件/SMI/平台管理层
# 需要 root
set -u

TR=/sys/kernel/tracing
[ -d "$TR" ] || { echo "tracefs 未挂载"; exit 1; }
[ "$(id -u)" = 0 ] || { echo "需要 root"; exit 1; }

echo 0 > $TR/tracing_on
echo hwlat > $TR/current_tracer
echo 5 > $TR/tracing_threshold          # µs（超过这个才算一次事件）

# 调参（默认 window=1s / width=0.5s：每秒里用 0.5s 检测）
echo 1000000 > $TR/hwlat_detector/window 2>/dev/null || true
echo 500000  > $TR/hwlat_detector/width  2>/dev/null || true
echo per-cpu > $TR/hwlat_detector/mode   2>/dev/null || true   # 每核常驻（v6.6+）

echo "开始检测（低负载下采 60 秒）…"
echo 1 > $TR/tracing_on
sleep 60
echo 0 > $TR/tracing_on

echo "===== 结果 ====="
cat $TR/trace
echo "本轮最大停顿: $(cat $TR/tracing_max_latency) µs"

echo
echo "===== 清理 ====="
echo nop > $TR/current_tracer
echo "读数：>10µs 的 hwlat 在 HFT 热核上就是事故；常见于 SMI（BIOS 电源管理/ECC 巡检），"
echo "      对策在 BIOS 里关 C-state/SMI 相关项或换平台——OS 层无解。"
