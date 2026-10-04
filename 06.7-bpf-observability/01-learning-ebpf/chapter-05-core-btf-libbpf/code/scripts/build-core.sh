#!/bin/bash
# build-core.sh — CO-RE 程序完整构建链（ch05 配套）
# 步骤: 生成 vmlinux.h → 编译内核侧 .bpf.o → 生成 skeleton → 编译用户态 loader
set -eu

cd "$(dirname "$0")/.."
BUILD=.build
mkdir -p "$BUILD"

echo "===== ① 从本机 BTF 生成 vmlinux.h（16 万行，放 .build/ 不入库） ====="
[ -f /sys/kernel/btf/vmlinux ] || { echo "内核无 BTF！运行 scripts/check-btf.sh 诊断"; exit 1; }
bpftool btf dump file /sys/kernel/btf/vmlinux format c > "$BUILD/vmlinux.h"
echo "生成 $BUILD/vmlinux.h（$(wc -l < "$BUILD/vmlinux.h") 行）"

echo
echo "===== ② 编译内核侧（CO-RE 重定位信息在 -g 里） ====="
clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
      -I"$BUILD" -I. -I/usr/include/x86_64-linux-gnu \
      -c hello-buffer-config.bpf.c -o "$BUILD/hello-buffer-config.bpf.o"
echo "生成 $BUILD/hello-buffer-config.bpf.o"

echo
echo "===== ③ 生成 skeleton（libbpf 骨架头，用户态 #include 用） ====="
bpftool gen skeleton "$BUILD/hello-buffer-config.bpf.o" > "$BUILD/hello-buffer-config.skel.h"
echo "生成 $BUILD/hello-buffer-config.skel.h"

echo
echo "===== ④ 编译用户态 loader ====="
cc -O2 -g -Wall -I"$BUILD" -I. \
   -o "$BUILD/hello-buffer-config" hello-buffer-config.c \
   -lbpf -lelf -lz
echo "生成 $BUILD/hello-buffer-config"

cat <<EOF

完成。运行（需 root）：
  sudo $BUILD/hello-buffer-config
  # 另开终端执行命令触发 execve；root 触发会显示自定义消息 "Hi root!"

验证 CO-RE 可移植性（可选）：把 .bpf.o 拷到另一台 BTF 内核不同版本的机器，
直接跑——这就是 "Compile Once, Run Everywhere"。
EOF
