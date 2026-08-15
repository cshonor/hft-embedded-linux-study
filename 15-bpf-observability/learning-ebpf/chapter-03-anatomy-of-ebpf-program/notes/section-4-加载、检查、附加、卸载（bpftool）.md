# 加载、检查、附加、卸载（bpftool）

```
bpftool prog load hello.bpf.o /sys/fs/bpf/hello   # 加载 + pin 到 bpffs
bpftool prog list                                  # 540: xdp name hello tag d35b... gpl
bpftool prog show id 540 --pretty                  # JSON 全字段
bpftool net attach xdp id 540 dev eth0             # 挂到网卡
bpftool net detach xdp dev eth0                    # 摘除（程序仍在内核）
rm /sys/fs/bpf/hello                               # 删 pin 文件 = 卸载
```

**prog show 关键字段：**
| 字段 | 含义 |
|------|------|
| id | 加载时分配，进程内唯一 |
| tag | 指令的 **SHA 哈希**——同一程序重加载 tag 不变（id 会变） |
| bytes_xlated | 过验证器（可能被内核改写）后的字节码字节数 |
| bytes_jited / jited | JIT 后机器码字节数（例：96B 字节码 → 148B 机器码） |
| memlock | 锁定内存（不会被换出）——eBPF 内存必须常驻 |
| map_ids | 关联的 map（本例源码没有 map 却有 2 个——见全局变量） |
| btf_id | 有 BTF 信息块（`-g` 编译才有） |

**程序引用四方式**：id / name / tag / pinned path。name 和 tag 可重复，id 和 pin 路径唯一。

`bpftool prog dump xlated` 看验证后字节码（与 llvm-objdump 基本一致）；`dump jited` 看机器码（需要 bpftool 编译时带 libbfd）。

**XDP 事件的上下文特殊性**：trace 输出行首是 `<idle>-0`——包到达时**没有任何用户态进程与之关联**（第 2 章 syscall 事件则有 pid/comm）。这解释了为什么网络类程序拿不到 `bpf_get_current_pid_tgid()` 的有效值。
