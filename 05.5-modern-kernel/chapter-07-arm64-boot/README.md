# Ch7 ARM64 启动流程

> 来源: Bootlin ARM64 Training
> 对标旧书: ULK3 Ch2 (x86 启动, 已过时)

U-Boot → head.S 汇编 → start_kernel C 代码 → init 进程。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 7.1 ARM64 启动汇编阶段 (head.S) | `notes/01-arm64-boot-assembly.md` |
| 7.2 start_kernel C 代码初始化 | `notes/02-start-kernel-init.md` |

---

## HFT 关联

理解启动流程有助于在 bootargs 中正确设置 isolcpus/nohz_full 等 HFT 参数。
