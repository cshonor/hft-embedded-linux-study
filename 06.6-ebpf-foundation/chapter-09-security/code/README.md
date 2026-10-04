# ch09 code · BPF LSM 示例

> 《Learning eBPF》第 9 章：LSM 程序返回值非零 = **拒绝操作**（管控，不只是观测）。

## 程序

| 文件 | 出处 | 内容 | 验证（clang 18.1 -target bpf） |
|------|------|------|------|
| [lsm-path-chmod.bpf.c](./lsm-path-chmod.bpf.c) | 9.1 块1 补全 | LSM path_chmod 审计 + 拒绝 chmod 777 的管控演示 | 编译 ✓ |

## 本机运行前提（实测不满足）

- `cat /sys/kernel/security/lsm` → `lockdown,capability,landlock,yama,apparmor,ima,evm`
  —— **没有 bpf**！本机无法加载 LSM 程序，需 boot 参数 `lsm=...,bpf` 后重启
- 内核演进：`dentry.d_iname` → `d_name.name`（qstr），书上写法已按本机 7.0 修正（见文件注释）

## 要点

- LSM 程序 license 必须 `GPL`（用到的 helper 是 gpl_only）
- 返回 `-1` 时调用方得到 `EPERM`——写安全策略前先在"只观测"模式下跑稳
