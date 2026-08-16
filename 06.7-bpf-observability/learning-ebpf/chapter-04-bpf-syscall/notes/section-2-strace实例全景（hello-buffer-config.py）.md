# strace 实例全景（hello-buffer-config.py）

```
bpf(BPF_BTF_LOAD, ...)                         = 3   # 加载 BTF 数据 → fd 3
bpf(BPF_MAP_CREATE, {PERF_EVENT_ARRAY ...})    = 4   # output perf buffer → fd 4
bpf(BPF_MAP_CREATE, {HASH, key 4B, value 12B,
     max_entries=10240, btf_fd=3})             = 5   # config 哈希表 → fd 5
bpf(BPF_PROG_LOAD, {KPROBE, insn_cnt=44,
     insns=..., license="GPL", prog_btf_fd=3}) = 6   # 程序 → fd 6（验证失败返回负值）
bpf(BPF_MAP_UPDATE_ELEM, {map_fd=5, ...})      = 0   # 写 config 表项
```

**各字段细节：**
- BPF_BTF_LOAD：跨内核版本可移植的类型信息（第 5 章）；老内核看不到这条
- config map 的 `key_size=4`（u32 存 UID）、`value_size=12`（user_msg_t）、`max_entries=10240` 是 **BCC 默认值**（源码没写就是它）
- `btf_fd=3` 让 bpftool 能漂亮打印 key/value 结构
- `insn_cnt`：字节码指令条数
- `expected_attach_type=BPF_CGROUP_INET_INGRESS` 看着像网络程序？其实该字段只对部分程序类型有意义，kprobe 不用；这个值只是枚举表第一个（=0）的默认占位
- 文件描述符是**进程私有**的：hello 程序里 fd 5 = config map，bpftool 里同一 map 可能是 fd 3
