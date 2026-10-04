# ch05 code · CO-RE 完整程序（hello-buffer-config）

> 《Learning eBPF》第 5 章示例补全为**完整可构建的 CO-RE 工程**（内核侧 + 用户态 loader + 构建链）。
> 笔记中 CO-RE 机制原理块（`bpf_core_read` 宏展开、`bpf_core_relo` 结构，5.3 块7/8/9）为教学引用，不落盘。

## 文件

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [hello-buffer-config.h](./hello-buffer-config.h) | 5.3 块3/4/6 | 用户态/内核态共享结构（事件 data_t + 配置 user_msg_t） | 编译链 ✓ |
| [hello-buffer-config.bpf.c](./hello-buffer-config.bpf.c) | 5.3 块3/4/5/6 | CO-RE 内核侧：BTF 风格 map + `ksyscall/execve`（可移植附加点）+ 配置查询 | `clang -target bpf` 编译 ✓ |
| [hello-buffer-config.c](./hello-buffer-config.c) | 5.3 块10 补全 | 用户态 loader：open_and_load → attach → 写 my_config → perf_buffer 收事件 | `cc -lbpf` 编译 ✓ |

## scripts/

| 脚本 | 内容 | 验证 |
|------|------|------|
| [build-core.sh](./scripts/build-core.sh) | 完整构建链：BTF→vmlinux.h → .bpf.o → skeleton → loader（产物在 `.build/`，已 gitignore） | **全链实跑通过**（2026-10，libbpf 1.3.0） |
| [check-btf.sh](./scripts/check-btf.sh) | BTF 三查：/sys/kernel/btf/vmlinux + CONFIG_DEBUG_INFO_BTF + tracepoint format | `bash -n` ✓；本机 BTF 存在（7.1MB） |

## 运行

```bash
bash scripts/build-core.sh          # 构建（不需要 root）
sudo .build/hello-buffer-config     # 运行（需要 root）
# 另开终端执行命令触发 execve；root 触发会显示自定义消息 "Hi root!"
```

## CO-RE 要点（本章核心）

- `vmlinux.h` 从**本机** BTF 生成，只用于编译；运行时靠 `-g` 里的 CO-RE 重定位信息，
  由 libbpf 在 `open_and_load()` 时按**运行内核**的 BTF 修正字段偏移 → "Compile Once, Run Everywhere"
- `SEC("ksyscall/execve")` 让 libbpf 自动解析架构相关 syscall 名（比硬编码 `__x64_sys_` 可移植）
- map 用 BTF 风格 `SEC(".maps")` + `__uint/__type` 宏声明（区别于 BCC 的 `BPF_HASH` 宏）
