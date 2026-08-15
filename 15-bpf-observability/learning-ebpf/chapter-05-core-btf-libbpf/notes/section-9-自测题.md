# 自测题

1. BCC 运行时编译方案的五个问题是什么？哪一条对嵌入式是致命的？
2. `bpftool btf dump map name config` 输出里 `bits_offset=32` 的 value 字段为什么从 32 位处开始？
3. `struct {char c; u64 n;}` 在 BTF 里 size 是多少？为什么？
4. vmlinux.h 是从哪个文件生成的？它缺哪类信息需要手工补？
5. `bpf_core_read()` 与 `bpf_probe_read_kernel()` 的唯一差别是什么？哪个 Clang 内建函数触发了重定位条目的生成？
6. 为什么编译 eBPF 必须 `-O2`？为什么还要 `-D __TARGET_ARCH_$(ARCH)`？
7. `struct bpf_core_relo` 四个字段各是什么含义？libbpf 拿到它之后做了什么？
8. `__open()` 和 `__load()` 拆开用的典型场景是什么？加载后修改 `skel->data` 结果如何？
9. 两个 eBPF 程序要共享一个 map，怎么避免 map 被创建两次？
