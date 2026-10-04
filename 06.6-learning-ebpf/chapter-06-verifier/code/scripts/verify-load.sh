#!/bin/bash
# verify-load.sh — 编译三个 .bpf.c 并尝试加载，亲眼看 verifier 拒绝（ch06 配套）
# 用法: ./verify-load.sh          —— 编译（不需要 root）
#       sudo ./verify-load.sh     —— 编译 + 加载实验（反例被拒、修正版通过）
set -u

cd "$(dirname "$0")/.."
BUILD=.build
mkdir -p "$BUILD"

# vmlinux.h 复用 ch05 生成的，或现生成
if [ ! -f "$BUILD/vmlinux.h" ]; then
    CH05=../../chapter-05-core-btf-libbpf/code/.build/vmlinux.h
    if [ -f "$CH05" ]; then
        cp "$CH05" "$BUILD/vmlinux.h"
    else
        bpftool btf dump file /sys/kernel/btf/vmlinux format c > "$BUILD/vmlinux.h"
    fi
fi

echo "===== ① 编译三个程序（编译全部通过——verifier 在加载时才工作） ====="
for f in verifier-fail-null-deref verifier-fail-bounds verifier-pass-fixed; do
    clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
          -I"$BUILD" -I/usr/include/x86_64-linux-gnu \
          -c "$f.bpf.c" -o "$BUILD/$f.bpf.o" && echo "  $f: 编译 OK"
done

[ "$(id -u)" = 0 ] || {
    echo
    echo "（非 root，跳过加载实验。sudo $0 重跑看 verifier 拒绝现场）"
    exit 0
}

echo
echo "===== ② 加载反例①：NULL 解引用 → 应被拒绝 ====="
bpftool prog load "$BUILD/verifier-fail-null-deref.bpf.o" /sys/fs/bpf/vfail1 2>&1 | tail -5 || true
echo "（找 'invalid mem access' / 'map_value_or_null' 字样）"

echo
echo "===== ③ 加载反例②：边界差一格 → 应被拒绝 ====="
bpftool prog load "$BUILD/verifier-fail-bounds.bpf.o" /sys/fs/bpf/vfail2 2>&1 | tail -5 || true
echo "（找 'invalid access to map value' / 'out of bounds' 字样）"

echo
echo "===== ④ 加载修正版 → 应通过 ====="
bpftool prog load "$BUILD/verifier-pass-fixed.bpf.o" /sys/fs/bpf/vpass && echo "  ✓ 通过（pinned at /sys/fs/bpf/vpass）"
rm -f /sys/fs/bpf/vfail1 /sys/fs/bpf/vfail2 /sys/fs/bpf/vpass

echo
echo "===== ⑤ 想看 verifier 完整推理过程 ====="
echo "  bpftool prog load $BUILD/verifier-pass-fixed.bpf.o /dev/null -d  # -d 打 verifier log"
