# 02-ch05 code · bpftrace 深入

> 《BPF Performance Tools》第 5 章示例落盘。
> 笔记中 ply 替代工具（§15 块13）、安装细节（§3 块16-19）保留在笔记内。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [vfs_read_latency.bt](./vfs_read_latency.bt) | §7 块8 | 书上第一个完整程序：vfs_read 耗时直方图（配对三件套标准形态） | 语义解析 ✓（实跑需 root） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [bpftrace-oneliners-10.sh](./scripts/bpftrace-oneliners-10.sh) | §5 块22、§1 块14、§14 块9-12 | 单行精选：execve/openat 行为、syscall 四种聚合、hist/lhist/sum、周期打印 | `bash -n` ✓ |
| [bpftrace-env-check.sh](./scripts/bpftrace-env-check.sh) | §3 块15/20 | 环境五查：内核版本/配置五项/版本/冒烟/探针可见性 | `bash -n` ✓；①②③⑤实跑 ✓（本机全过） |
| [bpftrace-idioms.sh](./scripts/bpftrace-idioms.sh) | §10/11/12/13/17 awk 块 | 惯用法速查：无 else if、位运算三式、变量四形态、ksym、uaddr、配对三件套、printf 调试 | `bash -n` ✓ |

## 本章核心：配对三件套（§17 块7）

```
kprobe:fn     { @start[tid] = nsecs; }
kretprobe:fn  /@start[tid]/          # ① 出口侧过滤
{
    $ns = nsecs - @start[tid];
    delete(@start[tid]);              # ② 用完即删（防 tid 复用串味）
    @ns = hist($ns);                  # ③ 先删后聚合
}
END { clear(@start); }                # 兜底清理
```
