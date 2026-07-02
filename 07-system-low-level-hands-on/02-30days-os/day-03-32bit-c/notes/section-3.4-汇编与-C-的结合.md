## ④ 汇编与 C 的结合

**一句话：C 写 OS 主逻辑；汇编只包 C 写不了或不该写的 CPU 专属操作，链接后 C 当普通函数调用。**

承前：[§3.3](./section-3.3-32-位模式前期准备与导入-C-语言.md) — 32 位下 **`HariMain`** 用 C 跑；本节解决 **「C 不够用时谁补」**。

| 小节 | 内容 |
|------|------|
| [§3.4.1 为何 C 需要汇编包装](./section-3.4.1-为何C需要汇编包装.md) | C 搞不定的指令、链接模型 |
| [§3.4.2 `io_hlt` 与四文件分工](./section-3.4.2-io_hlt与工程分层.md) | **ipl / nasmhead / bootpack.c / asmfunc** 分工与链接 |
| [§3.4.3 16 切 32 与 call C 完整例子](./section-3.4.3-16切32与call-C完整例子.md) | **汇编搭台、C 唱戏**、[sec-3.4-minimal-16-to-32-call-c/](../code/sec-3.4-minimal-16-to-32-call-c/) |
| [§3.4.4 嵌入式/HFT 与何时用 asm](./section-3.4.4-嵌入式HFT与何时用汇编.md) | 对比、自检 |

代码：[sec-3.4-bootpack-asm-and-c/](../code/sec-3.4-bootpack-asm-and-c/)（四文件） · 极简示例：[sec-3.4-minimal-16-to-32-call-c/](../code/sec-3.4-minimal-16-to-32-call-c/) · IPL：[sec-3.1-ipl-int13-disk-load/](../code/sec-3.1-ipl-int13-disk-load/)

---

### 本段带走什么

```text
ipl.asm / nasmhead.asm  →  汇编搭台（读盘、切模式）
bootpack.c              →  C 唱戏（HariMain）
asmfunc.asm             →  C 调用的指令补丁（io_hlt）
         链接器拼成一个 bootpack
```

---

### 自检

- [ ] 说清 **C 为什么不能直接写 `HLT` / 切模式**（§3.4.1）
- [ ] 区分 **`ipl / nasmhead / asmfunc / bootpack.c`**（§3.4.2）
- [ ] 能口述 **16→32 例子里 `call kernel_main` 交接**（§3.4.3）
- [ ] 知道 **嵌入式/HFT vs OS 引导** 的 asm 比例（§3.4.4）

---

← [§3.3 32 位与 C](./section-3.3-32-位模式前期准备与导入-C-语言.md) · [§3.4.1 →](./section-3.4.1-为何C需要汇编包装.md) · [Day 3 README](../README.md)
