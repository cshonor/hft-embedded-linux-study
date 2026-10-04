# 02-ch17 code · BPF 生态工具速查

> 《BPF Performance Tools》第 17 章示例落盘。
> 本章全是外部生态（PCP/ebpf_exporter/kubectl-trace），本机均未安装，
> 以"需要时怎么搭"的备忘脚本形式归档。

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [ecosystem-cheatsheet.sh](./scripts/ecosystem-cheatsheet.sh) | §2/4/5 | PCP+pmdabcc / Cloudflare ebpf_exporter（BPF→Prometheus）/ kubectl-trace + 四档选型（单机/趋势/PCP/k8s） | `bash -n` ✓ |
