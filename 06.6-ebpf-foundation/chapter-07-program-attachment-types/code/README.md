# ch07 code · 程序附加类型示例

> 《Learning eBPF》第 7 章：返回探针两代对比。
> 笔记中 uprobe SEC 写法（7.1 块2）已并入文件注释。

## 程序

| 文件 | 出处 | 内容 | 验证（clang 18.1 -target bpf） |
|------|------|------|------|
| [kretprobe-vs-fexit.bpf.c](./kretprobe-vs-fexit.bpf.c) | 7.1 块1 补全 | `kretprobe`（只有返回值）vs `fexit`（入参+返回值同时在手），do_unlinkat 对比 | 编译 ✓ |

## 内核演进实录（本机 7.0.0-38 实测）

书上写法 `BPF_CORE_READ(name, name)` 在 7.0 内核**编译失败**：
`struct filename` 的 `name` 字段被挪进匿名嵌入的 `__filename_head`。
这正是 CO-RE 存在的理由——但匿名嵌入时 CO-RE 也要换写法（见文件注释，按子结构类型读）。

- 加载与 attach 需 root；vmlinux.h 复用自 ch05 `.build/`（已拷入本章 `.build/`）
- uprobe 挂库函数（块2）：`SEC("uprobe/usr/lib/.../libssl.so.3/SSL_write")`——
  路径带版本号，升级库就失效；生产建议 USDT 或运行时解析路径
