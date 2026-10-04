#!/bin/bash
# lang-observability.sh — 语言级观测：C 五式 + Java 工具族（02-ch12 §2-4）
# 出自: notes/section-2-C语言.md、section-3-Java-基础与符号.md、section-4-Java-BPF工具族.md
set -u

cat <<'EOF'
===== C 语言（§2 块1/2） =====
# uprobe 参数与返回值（readline 经典例）
bpftrace -e 'uprobe:/bin/bash:readline { printf("readline: %s\n", str(arg0)); }'
bpftrace -e 'uretprobe:/bin/bash:readline { printf("readline: %s\n", str(retval)); }'

# BCC 六连
funccount-bpfcc 'attach*'                             # 内核函数
funccount-bpfcc '/bin/bash:a*'                        # 二进制函数
funccount-bpfcc '/lib/x86_64-linux-gnu/libc.so.6:a*'  # 库函数
trace-bpfcc '/bin/bash:readline' '"%s", arg1'
stackcount-bpfcc -u '/bin/bash:readline'              # 用户栈
profile-bpfcc -U -F 49                                # 49Hz 用户栈采样

===== Java（§3/4 块3-7） =====
# ① JNI 函数计数（235 个，最高频通常是 Get/ReleasePrimitiveArrayCritical）
funccount-bpfcc '/usr/lib/jvm/*/libjvm.so:jni*'
funccount-bpfcc '/usr/lib/jvm/*/libjvm.so:jni_Call*'

# ② 符号映射 + 火焰图（jmaps 生成 java 符号表，profile 才能译出方法名）
jmaps; profile-bpfcc -afp <PID> 10 > out.profile.txt
flamegraph.pl --color-java --hash < out.profile.txt > out.svg

# ③ off-CPU
jmaps; offcputime-bpfcc -fp <PID> 10 > out.offcpu.txt

# ④ 主存增长画像
stackcount-bpfcc -p <PID> t:exceptions:page_fault_user

# ⑤ USDT（JVM 内嵌探针，需 -XX:+ExtendedDTraceProbes）
# method__compile__begin → JIT 编译追踪
# class__loaded → 类加载追踪
EOF
