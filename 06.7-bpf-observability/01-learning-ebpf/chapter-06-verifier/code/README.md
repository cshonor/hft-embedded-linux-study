# ch06 code · verifier 反例与修正版

> 《Learning eBPF》第 6 章：verifier 的拒绝现场需要 root 加载才能看到；
> 这里的三个程序**编译全部通过**——verifier 是内核加载时的第二道关卡。
> 笔记中 `bpf_func_proto`/ctx 布局等（6.2 块1/4/5）为教学引用，不落盘。

## 程序（对照组）

| 文件 | 出处 | 内容 | 验证（clang 18.1 -target bpf） |
|------|------|------|------|
| [verifier-fail-null-deref.bpf.c](./verifier-fail-null-deref.bpf.c) | 6.2 块3 | 反例①：map 查询不判空直接用 → 加载时 "invalid mem access 'map_value_or_null'" | 编译 ✓（拒于加载） |
| [verifier-fail-bounds.bpf.c](./verifier-fail-bounds.bpf.c) | 6.2 块2 | 反例②：`c <= sizeof(message)` 差一格越界 → 范围传播发现上界 12 | 编译 ✓（拒于加载） |
| [verifier-pass-fixed.bpf.c](./verifier-pass-fixed.bpf.c) | 对照 | 判空 + 严格小于 | 编译 ✓（加载应通过） |

## scripts/

| 脚本 | 内容 |
|------|------|
| [verify-load.sh](./scripts/verify-load.sh) | 编译三个程序；**sudo 跑**则用 `bpftool prog load` 逐一加载：两个反例被拒（附要找的报错关键词）、修正版通过、`-d` 看 verifier 完整推理 |

## 编译要点（实测踩坑）

`BPF_KPROBE_SYSCALL` 需要**三个头**，缺一个都编不过：

```c
#include <bpf/bpf_helpers.h>     // helper 与 map 宏
#include <bpf/bpf_tracing.h>     // BPF_KPROBE_SYSCALL 本体
#include <bpf/bpf_core_read.h>   // 宏展开依赖的 BPF_CORE_READ
```
