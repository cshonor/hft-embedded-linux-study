# 六类典型验证失败

### 3.1 helper 函数对本程序类型不可用

不同程序类型可用不同 helper。XDP 由网卡收包触发，没有用户态进程，所以 `bpf_get_current_pid_tgid()` 无意义：

```
16: (85) call bpf_get_current_pid_tgid#14
unknown func bpf_get_current_pid_tgid#14
```

"unknown func" ≠ 函数不存在，只是**对这种程序类型**不可用。

### 3.2 helper 参数类型不对

每个 helper 在内核里（`kernel/bpf/helpers.c` 等）有 `bpf_func_proto` 描述参数约束：

```c
const struct bpf_func_proto bpf_map_lookup_elem_proto = {
    .func      = bpf_map_lookup_elem,
    .gpl_only  = false,
    .pkt_access = true,
    .ret_type  = RET_PTR_TO_MAP_VALUE_OR_NULL,
    .arg1_type = ARG_CONST_MAP_PTR,      // 第1参必须是 map 指针
    .arg2_type = ARG_PTR_TO_MAP_KEY,     // 第2参必须是指向 key 的指针
};
```

把 `bpf_map_lookup_elem(&my_config, &uid)` 的第一参改成局部变量 `&data`——**编译通过**，加载报错：

```
27: (85) call bpf_map_lookup_elem#1
R1 type=fp expected=map_ptr
```

fp = frame pointer（栈上局部变量区）。验证器靠寄存器类型跟踪抓住了编译器抓不住的错误。

### 3.3 GPL 许可证不匹配

用了 `gpl_only=true` 的 helper（如 `bpf_probe_read_kernel`）但程序没声明 GPL 兼容 license：

```
37: (85) call bpf_probe_read_kernel#113
cannot call GPL-restricted function from non-GPL compatible program
```

前面调的非 GPL helper 不受影响，报错只指向第一个 GPL-restricted 调用。

### 3.4 越界访问内存

**XDP 包边界**：`ctx->data` ~ `ctx->data_end` 之间才可访问。`data_end++` 想骗过边界检查：

```
R3 pointer arithmetic on pkt_end prohibited
```

**数组下标 off-by-one**（全局 message 是 12 字节数组）：

```c
if (c <= sizeof(message)) { char a = message[c]; ... }   // c==12 时越界！
```

报错：

```
invalid access to map value, value_size=16 off=16 size=1
R2 max value is outside of the allowed memory range
```

报"map value"是因为全局变量用 map 实现（第 3 章）。日志回溯法：从报错往上找 `R1_w=inv(id=0, umax_value=12, ...)` → R1（存着 c）最大可达 12 → 而合法下标只有 0~11 → 定位到 `<=`。局部变量版同样错误报 `invalid variable-offset read from stack R2`。

### 3.5 解引用可能为 NULL 的指针

`bpf_map_lookup_elem()` 返回 `RET_PTR_TO_MAP_VALUE_OR_NULL`——查不到就是 NULL：

```c
p = bpf_map_lookup_elem(&my_config, &uid);
char a = p->message[0];        // 编译通过，验证失败
```

```
29: (71) r3 = *(u8 *)(r7 +0)
R7 invalid mem access 'map_value_or_null'
```

修复：显式 `if (p != 0) { ... }`。部分 helper 内置判空：`bpf_probe_read_kernel()` 的第三参就叫 `unsafe_ptr`，允许传可能为 NULL 的指针，helper 内部先检查再解引用。

### 3.6 上下文字段访问越权

tracepoint 上下文开头的公共字段（common_type/common_flags/common_preempt_count/common_pid）**不允许** eBPF 访问，只能访问 tracepoint 特有字段；读错报 `invalid bpf_context access`。
