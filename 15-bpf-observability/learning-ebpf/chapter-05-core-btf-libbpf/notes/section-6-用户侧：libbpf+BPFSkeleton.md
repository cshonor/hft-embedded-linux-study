# 用户侧：libbpf + BPF Skeleton

### 6.1 生成与本质

```
bpftool gen skeleton hello-buffer-config.bpf.o > hello-buffer-config.skel.h
```

骨架头文件里有：程序/map 的结构定义、一整套 `hello_buffer_config_bpf__*` 生命周期函数、以及末尾的 `__elf_bytes()` 函数——**ELF 字节被嵌进骨架**，生成后 .o 文件可删，可执行文件自带字节码（也可用 `bpf_object__open_file()` 直接从 ELF 文件加载，二选一）。

### 6.2 生命周期主流程

```c
skel = hello_buffer_config_bpf__open_and_load();  // open: 解析 ELF；load: 装入内核 + CO-RE 修复
err  = hello_buffer_config_bpf__attach(skel);     // 按 SEC() 自动附加
pb   = perf_buffer__new(bpf_map__fd(skel->maps.output), 8,
                        handle_event, lost_event, NULL, NULL);
while (true) err = perf_buffer__poll(pb, 100);    // 100ms 超时轮询
perf_buffer__free(pb);
hello_buffer_config_bpf__destroy(skel);
```

细节：
- **open 与 load 可拆开**：`__open()` → 改配置（如 `skel->data->c = 10` 初始化全局变量）→ `__load()`。加载之后再改 `skel->data->c` **无效**——骨架对象只是 ELF 信息的用户态副本
- 复用已有 map：`bpf_map__set_autocreate()` 关掉自动创建，`bpf_obj_get("/sys/fs/bpf/xxx")` 按 pin 路径拿 fd（典型场景：两个 eBPF 程序共享一个 map，只允许一方建）
- SEC() 没写全附加点时，用 `bpf_program__attach_kprobe / attach_xdp / ...` 手工附加
- `libbpf_set_print()` 注册 libbpf 日志回调
