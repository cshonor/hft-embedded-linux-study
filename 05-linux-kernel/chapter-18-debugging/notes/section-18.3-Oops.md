## ② Oops

**Oops** = 内核报告 **无法处理的异常**（如 **空指针解引用**）。

| 输出内容 | 错误信息 · **寄存器** · **调用栈 backtrace** |
|----------|-----------------------------------------------|

#### 致命程度

| 发生位置 | 后果 |
|----------|------|
| **中断上下文**、**idle (pid 0)**、**init (pid 1)** | 无法继续 → **`panic()`** · **整机挂死** |
| **普通用户进程** 上下文 | 通常 **杀死该进程** · 内核 **尝试继续** |

#### 解码 Oops

| 时代 | 工具 |
|------|------|
| 早期 | **`ksymoops`** + **`System.map`** — 手动 **地址 → 符号** |
| **2.6+ `kallsyms`** | `CONFIG_KALLSYMS` — 符号表编进内核 → **直接可读 backtrace** |

```
BUG: unable to handle kernel NULL pointer dereference
IP: [<ffffffffa0123456>] my_drv_ioctl+0x42/0x100 [mydrv]
PGD 0 oops: 0000 [#1] SMP
CPU: 3 PID: 4242 Comm: mytest Tainted: G      W
RIP: 0010:my_drv_ioctl+0x42/0x100 [mydrv]
Code: 48 8b 7f 08 48 85 ff ...        ← 出错指令前后的机器码
Call Trace:
 my_ioctl_handler+0x1a/0x20
 do_vfs_ioctl+0xb9/0x620
 ...
CR2: 0000000000000010                 ← 触发 fault 的线性地址
```

#### Oops 解剖表（逐行读什么）

| 字段 | 含义 | 用途 |
|------|------|------|
| `IP:`/`RIP:` | 出错指令地址 + 符号（`函数+偏移/大小`） | **第一定位点** |
| `Code:` | 出错指令前后 ~20 字节机器码 | 无符号时手动 `objdump -d` 对齐 |
| `Call Trace` | 调用栈（从出错点向上） | 还原"怎么走到这一步" |
| `CR2` | 缺页线性地址（x86 控制寄存器 2） | NULL 解引用时通常是小偏移（如 0x10 = `NULL->field`）——**从偏移反推结构体字段** |
| `[#1]` | Oops 计数（同 boot 内第几次） | 多次 Oops = 连锁损坏，只信第一次 |
| `[mydrv]` 方括号 | 来自**模块**而非 vmlinux | `gdb vmlinux` 查不到——要用 `objdump -d mydrv.ko` / modinfo 载入地址 |
| `Tainted: G W` | 污染标志位 | G=专有模块 W=警告过……上游报 bug 前先解释字母 |

#### 定位工作流

```
1) RIP 地址 ──► gdb vmlinux
                  (gdb) list *(my_drv_ioctl+0x42)     ← 直接给出源码行
2) 无 CONFIG_DEBUG_INFO？──► addr2line -e vmlinux <addr>
3) 栈里模块地址 ──► gdb mydrv.ko（配 /proc/modules 的载入基址换算）
4) CR2=0x10 ──► 找到 my_dev 结构里偏移 0x10 的字段名──► 哪条路径会传 NULL？
```

| 时代 | 工具 |
|------|------|
| 早期 | **`ksymoops`** + **`System.map`** — 手动 **地址 → 符号** |
| **2.6+ `kallsyms`** | `CONFIG_KALLSYMS` — 符号表编进内核 → **直接可读 backtrace** |
| 现代 | kdump 落 vmcore 后用 **crash** 命令行交互分析（`bt`/`struct`/`rd`） |

> kallsyms 的取舍：编进符号表让 Oops 直接可读，但暴露内核布局（绕 KASLR）——发行版默认开，安全敏感场景注意 `CONFIG_KALLSYMS_ALL` 与 kptr_restrict 的搭配。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Oops 信息中最重要的字段是什么？如何从 Oops 定位源码？

<details><summary>答案</summary>

关键字段：1) RIP（出错指令地址）→ addr2line 或 objdump 定位源码行；2) Call Trace（调用栈）→ 定位调用链；3) Code:（出错指令前后的十六进制）→ 反汇编。`addr2line -e vmlinux <RIP地址>` 定位源码行。`gdb vmlinux` + `list *(RIP地址)` 查看源码。HFT 驱动 Oops 分析是最基本的内核排障技能。

</details>

**Q2.** Oops 和 panic 的区别？什么时候 Oops 不会变成 panic？

<details><summary>答案</summary>

Oops = 杀死出错进程/线程，系统可能继续运行（如果损坏不严重）。panic = 系统停止，不可恢复。Oops → panic 的条件：1) 在中断上下文中出错（无进程可杀）；2) 损坏关键内核数据结构；3) panic_on_oops 设置为 1。生产 HFT 系统通常设 panic_on_oops=1（损坏的内核不安全，宁可重启）。

</details>

</details>
---
