# 坑点清单

1. **syscall 入口探针不能当安全工具用**——TOCTOU 可被"幻影攻击"绕过；观测用途没问题（第 7 章伏笔的完整答案）
2. **seccomp profile 覆盖错误路径**：压力/故障场景触发的 syscall 若不在 profile 里，应用会在最需要报警的时候死掉
3. `seccomp_unotify` 官方明确不可用于安全策略——别拿它做防护
4. **BPF LSM 需要 5.7+ 且启用** `lsm=bpf`；生产前先确认内核配置
5. 挂内核内部函数（Tetragon 式）没有稳定保证——升级内核要回归测试；选挂点必须选"数据结构已填充完"的位置
6. Sigkill 策略先审计后强制；从审计切到强制前复盘所有历史告警
7. seccomp profile 只能在进程启动时应用——对已运行进程要用 syscall 追踪类工具（Falco/Tetragon）
