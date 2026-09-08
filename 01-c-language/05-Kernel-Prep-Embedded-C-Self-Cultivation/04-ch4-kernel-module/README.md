# CH4 · 内核模块与子系统的 GNU C 应用

**GNU C in the Linux Kernel**

> 一句话概括这一章：**把 CH1/CH2 的工具，放回它真正被用的地方去看。**
> CH3 是「没有 OS 的裸机」；这一章是「有内核的情况下，同一套扩展怎么被用到极致」。

> **前置**：[CH1](../01-ch1-gnu-c-basics/) · [CH2](../02-ch2-gnu-c-advanced/)
> **配合阅读**：[CH3 嵌入式驱动实战](../03-ch3-embedded-driver/)（同一个 `packed` 结构体，在裸机和驱动里的用法差别）

## 本章讲什么

CH1/CH2 告诉你**有这些工具**。但你第一次打开 `include/linux/list.h` 或 `kernel/module.c` 时仍然会懵——因为它们是**组合使用**的。

这一章回答三个具体问题：

1. **内核为什么几乎不用链表库而要 `container_of` 反推结构体？**
2. **`module_init()` 没有任何地方调用它，它到底什么时候跑？**
3. **一个 `.ko` 从 `insmod` 到 `.init` 段被释放，中间发生了什么？**

第 3 问的答案跟 CH2 的 [6.6 section](../02-ch2-gnu-c-advanced/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) 是同一件事：**用 `__attribute__((section))` 把一批函数指针放进同一个段，链接器自动收集成一张表。**

## 扩展 → 内核出处 速查表

这张表是本章的骨架。左边是 CH1/CH2 学过的扩展，右边是它在内核里的**真实落点**：

| 扩展（出处） | 内核里的样子 | 解决什么问题 |
|---|---|---|
| [`typeof`](../01-ch1-gnu-c-basics/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | `min()`/`max()`/`swap()`、`ALIGN()` | 宏要能「看清」实参类型，避免二次求值副作用 |
| [`container_of`](../01-ch1-gnu-c-basics/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | `struct list_head`、`for_each_*` 系列 | **侵入式链表**：链表节点嵌在被管理对象里，由成员地址反推宿主地址 |
| [语句表达式](../01-ch1-gnu-c-basics/6.3-statement-expr/6.3-宏构造-利器-语句表达式.md) | `min()` 的类型安全实现 | 宏里需要临时变量又不能污染外层作用域 |
| [柔性数组](../01-ch1-gnu-c-basics/6.5-zero-length-array/6.5-零长度数组.md) | `skb` 的线性区、`hid_report` | 头部结构体 + 变长负载一个 `kmalloc` 搞定 |
| **[指定初始化](../01-ch1-gnu-c-basics/6.2-designated-init/)** | `struct file_operations xxx_fops = { .owner = THIS_MODULE, .read = xxx_read, };` | **这是驱动最日常的写法**：结构体字段几十个，只填用到的 |
| [`packed`](../02-ch2-gnu-c-advanced/6.7-aligned/6.7-属性声明-aligned.md) | `struct tcphdr`、各种协议头 | 报文结构体必须字节级贴合线上格式 |
| [`aligned`](../02-ch2-gnu-c-advanced/6.7-aligned/6.7-属性声明-aligned.md) | cache line 对齐的 per-CPU 数据 | 避免 false sharing |
| [`section` / `initcall`](../02-ch2-gnu-c-advanced/6.6-section/) | `module_init()` → `.initcall6.init` 段 | **不写注册代码**：链接器按段收集函数指针 |
| [`weak`](../02-ch2-gnu-c-advanced/6.9-weak/6.9-属性声明-weak.md) | `__weak` 的默认钩子、`arch/*` 覆写 | 架构无关代码留默认实现，具体架构可覆盖 |
| [`likely`/`unlikely`](../02-ch2-gnu-c-advanced/6.11-builtin/6.11.6-Linux内核中的likely和unlikely.md) | 全内核热点分支 | 分支概率提示，影响代码布局 |
| [`format`](../02-ch2-gnu-c-advanced/6.8-format/6.8-属性声明-format.md) | `printk()` 的定义 | 编译期检查格式串与参数类型是否匹配 |
| [`alias`](../02-ch2-gnu-c-advanced/6.9-weak/6.9.4-属性声明-alias.md) | `__attribute__((alias("__x")))` 符号别名 | 同一个实现挂多个符号名 |
| 内联汇编 [3.6](../02-ch2-gnu-c-advanced/3.6-mixed-programming/) | `rmb()`/`wmb()`、`cpu_relax()`、原子操作 | 编译器不该优化掉的那些精确动作 |

## 章节导航

| 序 | 小节 | 状态 | 内容 |
|----|------|------|------|
| 1 | [4.10 Linux 内核模块运行机制](./4.10-Linux内核模块运行机制.md) | 骨架 2 KB | `.ko` 的加载流程、`ELF` 视角下的模块、符号导出与版本校验 |
| 2 | [4.11 Linux 内核编译和启动分析](./4.11-Linux内核编译和启动分析.md) | 骨架 2 KB | `vmlinux` 的构建、`.init` 段的生与死 |
| 3 | [4.12 U-Boot 重定位分析](./4.12-U-boot重定位分析.md) | 骨架 2 KB | Bootloader 怎么给自己搬家；与 [6.6.3 U-Boot 镜像自复制](../02-ch2-gnu-c-advanced/6.6-section/6.6.3-U-boot镜像自复制分析.md) 配合 |

## 补写计划（按优先级）

本章目前是**骨架状态**（3 × 2 KB），是六章里唯一需要大量补写的。建议顺序：

| 优先级 | 主题 | 为什么先写它 |
|--------|------|-------------|
| ★★★ | **`initcall` 机制全解** | 唯一把 CH2 的 `section` 用到出神入化的地方；搞懂它，`module_init` 就再也不是黑魔法 |
| ★★★ | **`container_of` 与侵入式链表** | CH1 讲了宏本身，这里讲内核为什么这么设计（对比「链表挂数据」的常规做法） |
| ★★☆ | **`.ko` 的加载与符号决议** | 接 4.10；本质是 CH2 的强弱符号 + CH6 的动态链接 |
| ★★☆ | **协议结构体的 `packed` 实践** | 接 4.10 + CH2 6.7；看 `struct tcphdr` 怎么处理位域与字节序 |
| ★☆☆ | **per-CPU 与 `aligned`** | 需要一点调度背景，见 [附录 A](../90-ref-os/) |
| ★☆☆ | U-Boot 重定位 | 深度可选，做 bootloader 才用得上 |

> 前三项补完，读 LKD「设备驱动模型」一章就不会卡了。

## 与 CH3 的分工

同一件事，两章视角不同：

| 场景 | [CH3 裸机](../03-ch3-embedded-driver/) | **CH4 内核** |
|------|------------------|------------|
| 寄存器访问 | 自己 `volatile` 指针 | `ioremap()` + `readl/writel` 封装 |
| 中断 | 自己写 ISR、自己判共享数据 | `request_irq()` + 上下半部 |
| 初始化表 | 手工函数指针数组 | `module_init` + 链接器段 |
| 数据结构 | 静态全局 + 柔性数组 | `container_of` + `list_head` |
| 内存信息来源 | 看 [CH6 链接脚本](../06-ch6-toolchain-custom/4.14-链接脚本.md) | `kmalloc` / `vmalloc` / slab |

> **CH3 是「自己做一遍」，CH4 是「看内核为什么这么做」。** 先有 CH3 的手写版，再看 CH4 的封装才有收获。

<details><summary>代码自测（点击展开）</summary>

**Q1：`module_init(fn)` 展开后到底发生了什么？**

<details><summary>答案</summary>
它不是函数调用，而是**放置**。展开后大致等价于：

```c
static initcall_t __initcall_fn6 __used __attribute__((__section__(".initcall6.init"))) = fn;
```

也就是把 `fn` 的地址放进 `.initcall6.init` 这个段。链接脚本里把这个段的起止地址导出为 `__initcall_start[]`/`__initcall_end[]`，内核启动时用 `for (call = __initcall_start; call < __initcall_end; call++) (*call)();` 依次执行。

**所以 `module_init` 从来没有被谁显式调用过**——它是被链接器和启动循环找出来的。这就是 CH2 所说的「链接期语义」。
</details>

**Q2：为什么内核的链表是「链表节点嵌在结构体里」，而不是「结构体指针挂在链表上」？**

<details><summary>答案</summary>
后者需要每种数据类型各写一份链表实现（或用 `void*` 丢掉类型）。前者只写一份 `struct list_head` 的通用操作，用 `container_of` 从成员地址反推宿主地址，**同时保留完整类型信息**。

代价：一个对象只能同时属于一条 `list_head` 链表（需要多条链表就要多个 `list_head` 成员——所以 `struct task_struct` 里有好几个）。
</details>

**Q3：`__init` 修饰的函数/数据最后去哪了？**

<details><summary>答案</summary>
它们被放进 `.init.text` / `.init.data` 段。内核启动完成后会调用 `free_initmem()` 把整个段的页释放掉，回收内存（嵌入式上能回收几百 KB 到几 MB）。

副作用：这些函数**只能调用一次**。已经释放的地址如果再被调到，就是踩野内存——这也是驱动里 `__init` 不能乱标的原因。
</details>

</details>
