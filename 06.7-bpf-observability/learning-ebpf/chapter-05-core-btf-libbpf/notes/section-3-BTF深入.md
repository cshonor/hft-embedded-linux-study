# BTF 深入

### 3.1 BTF 不只为 CO-RE 服务

- **漂亮打印**：bpftool 用 BTF 把 map 里的字节按类型还原成人话（第 4 章已见）
- **源码交错**：`bpftool prog dump` 里 C 源码与指令交错、第 6 章验证器日志带源码，都靠 BTF 的行/函数信息
- **BPF 自旋锁**（5.1 起）：`struct bpf_spin_lock` 必须内嵌在 map value 结构里，内核需要 BTF 才知道锁字段在哪。限制：只能用于 hash/array map，不能用于 tracing 和 socket filter 程序

### 3.2 bpftool 查看 BTF

```
bpftool btf list          # 所有已加载 BTF：第 1 项 vmlinux（约 5.8MB）
bpftool btf dump id 149   # 某个 BTF blob 的全部类型定义
bpftool btf dump map name config    # 只看某个 map 相关联的类型
bpftool btf dump prog <id>          # 只看某个程序的
```

`btf list` 里每行可见：BTF id、大小、关联的 prog_ids / map_ids / pids。注意 **perf event buffer map 不使用 BTF**，所以 map_ids 列表里看不到它。

### 3.3 读懂 BTF 类型定义（书上手把手例子）

源码里 `BPF_HASH(config, u32, struct user_msg_t)`，`user_msg_t` 含 12 字节 message。BTF dump：

```
[1] TYPEDEF 'u32' type_id=2
[2] TYPEDEF '__u32' type_id=3
[3] INT 'unsigned int' size=4 bits_offset=0 nr_bits=32 encoding=(none)
[4] STRUCT 'user_msg_t' size=12 vlen=1
        'message' type_id=6 bits_offset=0
[5] INT 'char' size=1 bits_offset=0 nr_bits=8 encoding=(none)
[6] ARRAY '(anon)' type_id=5 index_type_id=7 nr_elems=12
[8] STRUCT '____btf_map_config' size=16 vlen=2      ← BCC 自动生成的 key+value 包装结构
        'key' type_id=1 bits_offset=0
        'value' type_id=4 bits_offset=32
```

解读要点：
- 每行 `[N]` 是类型 id，类型间用 `type_id=` 链式引用（u32 → __u32 → unsigned int 三层 typedef）
- `vlen` = 结构体字段数；`bits_offset` = 字段在结构内的位偏移
- **对齐坑**：`{char letter; u64 number;}` 中 letter 后面有 7 字节填充（64 位对齐），所以不能假设字段紧挨着；`____btf_map_config` 里 value 从 32 位处开始正是因为 key 占前 32 位
- 函数也有 BTF：`FUNC_PROTO`（返回类型 + 参数）+ `FUNC`；`PTR type_id=0` = void 指针

### 3.4 map 创建时如何携带 BTF

`bpf(BPF_MAP_CREATE)` 的 attr 里有 `btf_fd / btf_key_type_id / btf_value_type_id` 三个字段。BTF 之前，内核只知道 key/value 各占多少字节（`key_size/value_size`），不知道内部结构。注意 key 和 value 是**分开传两个 type_id**，`____btf_map_config` 只是 BCC 用户态的产物，内核不用它。
