#!/bin/bash
# bpftrace-idioms.sh — bpftrace 惯用法速查（02-ch05 §10/11/12/13/17 awk 块）
# 展示用（不是可执行实验）；可运行示例见 ../vfs_read_latency.bt
set -u

cat <<'EOF'
===== ① 控制流：没有 else if（§10 块1） =====
if ($f == 1) { ... } else {
    if ($f == 2) { ... } else {
        if ($f == 3) { ... }
    }
}
# 三层以上就该重构：换 map 查表或拆成多个探针/过滤器

===== ② 位运算三式（§11 块2） =====
args->flags & O_NONBLOCK          # 标志位测试：按位与，非零为真
(args->gfp_mask >> 1) & 0x3       # 位域抽取：移位 + 掩码
$addr & 0xFFF                     # 对齐判断：非零 = 不是页对齐

===== ③ 变量四种形态（§12 块3） =====
@start = nsecs;              # 全局整数
@last[tid] = nsecs;          # map：键整数，值整数
@bytes = hist(retval);       # 特殊类型：2 的幂直方图
@who[pid,comm] = count();    # 复合键（int,string）→ count

===== ④ 地址 → 符号（§13 块6） =====
@[args->function] = count()        # 打出来是裸地址
@[ksym(args->function)] = count()  # ksym() 渲染成函数名

===== ⑤ 用户态地址读字符串（§13 块7） =====
uprobe:/bin/bash:readline { printf("PS1: %s\n", str(*uaddr("ps1_prompt"))); }
# uaddr() 读用户态全局变量；retval 在 uretprobe 里是返回值

===== ⑥ 配对三件套（§17 块7，防 bug 标准形态） =====
kprobe:fn     { @start[tid] = nsecs; }
kretprobe:fn  /@start[tid]/          # ① 出口侧过滤
{
    $ns = nsecs - @start[tid];
    delete(@start[tid]);              # ② 用完即删（防 tid 复用串味）
    @ns = hist($ns);                  # ③ 先删后聚合
}
END { clear(@start); }                # 兜底清理

===== ⑦ 调试：printf 是你的 printk（§17 块6） =====
printf("%d duration_ms=(%d-%d)/1000000\n", tid, nsecs, @start[tid]);
# bpftrace -d 看 AST，-v 看 verifier/LLVM IR
EOF
