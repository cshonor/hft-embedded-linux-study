# 坑点清单

1. **XDP 返回 0 = XDP_ABORTED ≠ 成功**——返回 0 挂到 eth0 会丢掉所有包，SSH 直连机器直接失联。测试 XDP 放容器/虚拟网卡里（参考 lizrice/lb-from-scratch）
2. 忘了 `SEC("license")` 或用了 GPL-only helper 却声明专有许可 → 验证器拒绝
3. 编译不加 `-g`：没 BTF → bpftool 无法漂亮打印、CO-RE 不可用（第 5 章）
4. bpftool 必定 pin；忘删 pin 文件 = 程序滞留内核（重启前一直占内存）
5. `net.core.bpf_jit_enable` sysctl / `CONFIG_BPF_JIT` 控制 JIT——性能测试前先确认开着
6. tag 与 id 的区别：tag 跟着内容走（内容变 tag 变），id 跟着加载实例走
