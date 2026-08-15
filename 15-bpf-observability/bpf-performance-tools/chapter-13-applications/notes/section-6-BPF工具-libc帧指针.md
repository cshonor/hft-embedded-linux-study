# 6. BPF 工具：libc 帧指针问题（13.2.9）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.9 节（印刷 p644–645）

## 问题：栈在 libc 处断裂

ioprofile(8) 输出**完整堆栈**的前提是 MySQL 运行在**编译了帧指针的 libc** 上。而现实中：

- 应用程序通常**经由 libc 做 I/O 调用**
- libc **通常没有编译帧指针**
- → 从内核回到应用程序的堆栈遍历**通常停止在 libc 处**

问题对 ioprofile 最明显（第 7 章的 brkstack(8) 也存在）。症状——mysqld 有帧指针但用的是标准 libc 包：

```
# ioprofile.bt $(pgrep mysqld)
@[tracepoint:syscalls:sys_enter_pwrite64,
    pwrite+79
    0x2ffffffdc020000          ← 栈断在这里
    mysqld]: 5

@[tracepoint:syscalls:sys_enter_recvfrom,
    libc recv+94               ← 只显示一两帧就停
    mysqld]: 22526
```

## 四种修复方法

1. **用 -fno-omit-frame-pointer 重新编译 libc**
2. **跟踪 libc 接口方法**——在 libc 重用帧指针寄存器之前（库函数入口处）
3. **跟踪应用程序自己的函数**，如 MySQL 的 os_file_io()（应用函数是编译了帧指针的）
4. **用另外的堆栈遍历器**（2.4 节总结了其他方法：如 DWARF、ORC、last branch record 等）

## 背景

- libc 在 glibc 包中，该包还提供 libpthread 等库
- 曾经有建议：Debian 提供一个带帧指针的 libc 替代包
- 更多不完整堆栈的讨论见 2.4 节和 18.8 节

## HFT 关联

- 这是自建交易基础设施（自编译内核/库）最有性价比的改动之一：**自编译 glibc/libpthread 加 -fno-omit-frame-pointer**，让生产环境所有 ustack 工具开箱即用；代价是少量寄存器与性能开销（帧指针占用一个寄存器），对延迟敏感路径需评估
- 若不能重编 libc，第 2/3 法（跟踪 libc 入口或应用自有函数）是 BPF 侧的零成本替代——对策略代码直接 uprobe 自己的函数，绕开 libc 断层

<details>
<summary>自测题</summary>

1. 为什么堆栈遍历会在 libc 处停止？
   <details><summary>答</summary>发行版 libc 编译时省略了帧指针，BPF 无法靠帧指针链继续回溯。</details>

2. 不重编 libc 的两种补救？
   <details><summary>答</summary>跟踪 libc 接口方法（帧指针寄存器尚未被重用的入口处）；或直接跟踪应用程序自己的函数（应用带帧指针）。也可换用其他堆栈遍历器。</details>
</details>
