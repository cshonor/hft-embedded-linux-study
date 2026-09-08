# demo/09-builtin

对应 [6.11 内建函数](../../6.11-builtin/6.11-内建函数.md)。

## 文件

| 文件 | 主题 |
|------|------|
| `t1_bitops.c` | popcount/clz/ctz/ffs/parity — 指令映射与常量折叠 |
| `t2_bswap.c` | bswap16/32/64 — 字节序 |
| `t3_constant_p.c` | `__builtin_constant_p` — -O0 vs -O2 差异 |
| `t4_expect.c` | `__builtin_expect` — 基本块布局对比 |
| `t5_prefetch.c` | `__builtin_prefetch` — 预取指令映射 |
| `t6_control.c` | `__builtin_trap`/`unreachable`/`abort` |
| `t7_libc.c` | strlen/memcpy/strcmp 编译期折叠 |
| `t8_overflow.c` | `__builtin_add/sub/mul_overflow` |
| `t9_types.c` | `__builtin_types_compatible_p`/`assume_aligned`/`offsetof` |
| `t10_fno_builtin.c` | `-fno-builtin` / `-fno-builtin-strlen` 效果 |
| `t11_matrix.c` | GCC vs Clang 支持矩阵 |
| `t12_c23.c` | C23 `__builtin_stdc_*` |
| `t13_popcnt.c` | `-mpopcnt` 对 popcount 指令的影响 |

## 运行

```bash
bash run.sh
```

环境：WSL · gcc 13.3 / clang 18.1.3 · x86-64
