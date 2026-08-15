# CO-RE 五要素

| 要素 | 作用 |
|---|---|
| **BTF** | 描述数据结构与函数签名的格式；用来对比"编译时布局"与"运行时布局"的差异。5.4 起内核自带（需 `CONFIG_DEBUG_INFO_BTF`） |
| **内核头文件** | 不再逐个 include 内核头文件，用 `bpftool` 生成一个 `vmlinux.h` 全量搞定 |
| **编译器支持** | Clang 加 `-g` 编译时生成 CO-RE 重定位信息（GCC 12 起也支持 BPF 目标的 CO-RE） |
| **重定位库** | 加载时改写字节码适配目标内核：C 用 libbpf，Go 用 cilium/ebpf，Rust 用 Aya |
| **BPF skeleton**（可选） | `bpftool gen skeleton` 自动生成的生命周期管理代码，比裸调库方便 |

必读材料（作者反复点名 Andrii Nakryiko）：CO-RE 博客、《BPF CO-RE Reference Guide》、libbpf-bootstrap 教程、BCC→libbpf 迁移指南。
