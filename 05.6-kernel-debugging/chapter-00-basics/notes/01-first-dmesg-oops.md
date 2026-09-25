# 0.2 第一次看懂 dmesg：printk 与 Oops 骨架

> 🟢 零起点 · 本节零环境要求，会开终端就能跟

## 本节讲什么

两件事：① `dmesg` 是什么、内核日志怎么分级；② 内核崩的时候刷出来的一整屏
**Oops** 长什么样——不逐字段精读（那是 [Ch7](../chapter-07-oops/) 的活），
只学会认出它的**骨架**：哪一行是「死在哪」、哪一段是「怎么死的」。

> ⚠ 诚实标注：本节的 Oops 样例是**示意文本**（按原书 Ch7 的典型格式手工整理），
> 本机是 macOS 没有 dmesg 的 Linux 语义；Pi 5 上 `dmesg` 随手可得，照敲即可。
> 字段逐行的官方语义见 Ch7 的 07.2/07.3 两节笔记。

## 1. dmesg：内核的行车道

用户态程序用 `printf` 输出，内核用 **printk**。printk 的输出不直接怼到你屏幕上，
而是先进内核的环形缓冲区（ring buffer），`dmesg` 命令就是把这个缓冲区倒出来看：

```bash
$ dmesg | tail -5          # 看最新 5 条
$ dmesg --follow           # 实时滚动（类似 tail -f），Ctrl-C 退出
$ sudo dmesg --clear       # 清空（实验内核模块前先清，输出干净）
```

printk 有 8 个级别，数字越小越严重（记忆法：**0 最响，7 最碎语**）：

| 级别 | 宏 | 什么时候用 | dmesg 里的样子 |
|------|-----|-----------|----------------|
| 0 | KERN_EMERG | 系统要死了 | 一般看不到，看到了已经 panic |
| 3 | KERN_ERR | 出错了（设备打开失败等） | 正常红字级别，`dmesg --level=err` 可单看 |
| 4 | KERN_WARNING | 可疑但不致命 | Oops 前奏常见 |
| 6 | KERN_INFO | 普通信息（驱动加载成功） | 绝大多数日志 |
| 7 | KERN_DEBUG | 碎语，默认可能不显示 | 动态调试的战场（Ch3） |

新手只需要记住：**自己写模块调试，用 `pr_info()`（= KERN_INFO）起步**；
正式判断问题先 `dmesg --level=err,warn` 过滤。

## 2. Oops 骨架：一屏文字，三段结构

写内核模块时解引用了一个空指针（用户态这就是段错误，内核态就是 Oops），
`dmesg` 里会多出类似下面这一屏（**示意**，精简到新手必看的行）：

```console
BUG: kernel NULL pointer dereference, address: 0000000000000000   ← ① 死因
#PF: supervisor read access in kernel mode
#PF: error_code(0x0000) - not-present page
PGD 0 P4D 0
Oops: 0000 [#1] SMP NOPTI                                            ← ② 死状
CPU: 2 PID: 1234 Comm: mybug Tainted: G      O  6.1.0-mykernel
Hardware name: Raspberry Pi 5 ...
RIP: 0010:mybug_read+0x1e/0x40 [mybug]                             ← ③ 死在哪 ★
Code: ...
RSP: 0018:ffffc900002f3de8 EFLAGS: 00010246
RAX: 0000000000000000 RBX: ...
Call Trace:                                                          ← ④ 怎么走到这的
 <TASK>
 mybug_read+0x1e/0x40 [mybug]
 vfs_read+0x8c/0x140
 ksys_read+0x5f/0xe0
 do_syscall_64+0x3b/0x90
 entry_SYSCALL_64_after_hwframe+0x63/0xcd
 </TASK>
---[ end trace 0000000000000000 ]---
```

新手只认四样东西：

| 段 | 认什么 | 一句话 |
|----|--------|--------|
| ① 死因 | 第一行 `BUG: ...` | 崩溃类型 + 出错的虚拟地址。`NULL pointer dereference` = 解引用空指针（新手 80% 的第一个 Oops 都是它） |
| ③ 死在哪 ★ | `RIP:` 行 | **最重要的一行**。`mybug_read+0x1e/0x40 [mybug]` = 死在模块 mybug 的 `mybug_read` 函数里、入口偏移 0x1e、整个函数 0x40 字节。`[方括号]` 里的名字 = 出事的模块——如果你的模块名在这，凶手基本就是你了 |
| ④ 死法 | `Call Trace:` | 调用链：从下往上看 = 「系统调用 read → vfs_read → 你的 mybug_read → 死」。这是「怎么走到这一步的」 |
| 结束线 | `end trace` | Oops 的收尾标记，之后的日志是别的事了 |

两句话读法：**先看 RIP 找凶手，再看 Call Trace 找动机**。

## 3. Oops 之后系统死了吗？——Oops vs panic

新手最容易混的一对词（精读在 [Ch7 的 07.1](../chapter-07-oops/notes/01-oops-vs-panic.md)）：

| | Oops | panic |
|---|------|-------|
| 比喻 | **伤了**（骨折） | **死了**（宣布死亡） |
| 范围 | 杀死出错的**任务**（进程/线程），内核大体还活着 | 内核自己停下来，全机冻结，只能重启 |
| 屏幕上 | dmesg 多一屏 trace，shell 还能用 | 整屏刷满、光标不再动、Ctrl-Alt-Del 都可能没反应 |
| 常见触发 | 模块解引用坏指针（可恢复的错） | 中断上下文里崩溃、关键路径自检失败 |

为什么有这个区别：内核对「进程上下文里的错误」有一定恢复能力（把出错的
task 干掉继续跑）；但对「内核自身核心数据结构不一致」没有恢复方案，硬着头皮跑
会损坏更多数据，宁可立刻停机——这就是 panic 的设计哲学：**快速失败**。

## 4. 新手动手清单（有一台 Linux 就行，Pi 5 完美）

1. `dmesg | head -20`——看开机日志，找到 `Linux version` 那行（内核版本号在这）
2. `dmesg --level=err,warn`——只看警告和错误，数量通常远比想象少
3. `echo` 一个 Oops 出来（可选）：装本模块 Ch7 例子里的坏模块跑一次，
   或者搜索 `oops example module` 现成源码——重点是把上面的骨架对照真实输出认一遍
4. 把本节四样东西的口诀抄下来：**第一行死因、RIP 找凶手、Call Trace 看路径、end trace 收尾**

## 与后续衔接

- RIP 那行 `+0x1e/0x40` 的偏移怎么换算回源码行 → [Ch7](../chapter-07-oops/) 的 07.4 addr2line / 07.5 objdump
- printk 全家（级别、动态调试、`/proc/dynamic_debug`）→ [Ch3](../chapter-03-printk/)
- 下一节 [0.3](./02-glossary-path.md)：术语速查 + 12 章怎么啃
