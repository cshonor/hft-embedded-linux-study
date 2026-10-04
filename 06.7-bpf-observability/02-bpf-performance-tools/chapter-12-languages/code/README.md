# 02-ch12 code · 语言级观测

> 《BPF Performance Tools》第 12 章示例落盘。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [lang-observability.sh](./scripts/lang-observability.sh) | §2-4 | C 语言六连（uprobe 参数/返回值 + funccount 三域 + stackcount/profile）+ Java 工具族五步（JNI/jmaps/火焰图/off-CPU/USDT） | `bash -n` ✓；C 段工具已装 |

## 要点

- C 二进制/库函数用 `funccount '/path/to/bin:func*'` 形式（uprobe 通配）
- Java 必须 `jmaps` 先生成符号表，否则火焰图全是地址（JIT 代码不在 ELF 符号表里）
- JVM USDT 需启动参数 `-XX:+ExtendedDTraceProbes`
