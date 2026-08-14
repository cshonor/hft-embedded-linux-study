## ⑤ 内核模块 · Modules

Linux = **宏内核**，但支持 **可加载模块**（Loadable Kernel Modules, LKM）— 运行时 **插入/移除** 对象代码，兼顾宏内核性能和微内核灵活性。

---

### 模块的核心价值

| 作用 | 说明 |
|------|------|
| **设备驱动** | 按需 `modprobe` — 无硬件时不占内核内存 |
| **热插拔** | 总线探测 → 加载对应模块 → udev 自动处理 |
| **不限于驱动** | 文件系统（nfs.ko）、协议（ipv6.ko）、安全（selinux）均可模块化 |
| **HFT 定制** | 网卡驱动定制编译为 .ko，不修改主线内核源码 |

---

### 用户命令

| 命令 | 说明 |
|----------|------|
| **`insmod module.ko`** | 直接加载单个 .ko（不处理依赖） |
| **`modprobe module`** | 自动解析依赖，按顺序加载（推荐） |
| **`rmmod module`** | 卸载模块（引用计数为 0 时） |
| **`lsmod`** | 列出已加载模块（读 /proc/modules） |
| **`modinfo module.ko`** | 查看模块信息（作者/许可证/参数） |
| **`depmod`** | 生成 modules.dep 依赖数据库 |

```bash
# HFT 常见操作
modprobe ixgbe                  # 加载网卡驱动
cat /sys/module/ixgbe/parameters/  # 查看驱动参数
rmmod ixgbe && insmod ./ixgbe_custom.ko  # 换定制驱动（需停网卡）
modinfo ixgbe                   # 查看版本/参数/依赖
```

---

### .ko 文件的 ELF 结构

内核模块 .ko 本质是 **可重定位 ELF 目标文件**（`ET_REL`），和 .o 类似但多了 `.modinfo` 节。

```bash
# 用 readelf 查看 .ko 的 ELF 类型
readelf -h ixgbe.ko | grep Type
# Type: REL (Relocatable file)

# 查看节
readelf -S ixgbe.ko
```

| 节 | 内容 | 说明 |
|----|------|------|
| `.text` | 代码段 | 模块的函数实现 |
| `.data` | 已初始化全局变量 | |
| `.bss` | 未初始化全局变量 | |
| `.rodata` | 只读数据 | 字符串常量等 |
| `.symtab` | 符号表 | 函数名、变量名 |
| `.strtab` | 字符串表 | 符号名称字符串 |
| `.modinfo` | 模块元信息 | license、version、参数 |
| `.note.gnu.build-id` | 构建标识 | 调试关联 |

```bash
# 查看模块元信息（直接读 .modinfo 节）
readelf -p .modinfo ixgbe.ko

# 输出示例：
# license=GPL
# description=Intel(R) 10GbE PCI Express Linux Network Driver
# parm=num_queues:Number of queues (uint)
```

> **交叉引用：** ELF 文件结构详解见 C 笔记 `02-Pointers-on-C/ch18/18.4-ELF文件回顾.md`，readelf 全选项见 `18.6-readelf-ELF结构全景.md`。

---

### nm 分析内核模块符号

```bash
# 查看模块导出的符号
nm ixgbe.ko | grep ' T '
# T ixgbe_open
# T ixgbe_xmit_frame
# T ixgbe_intr

# 查看未定义符号（模块依赖的外部函数）
nm -u ixgbe.ko
# U __netdev_alloc_skb
# U dma_alloc_coherent
# U printk
# U register_netdev

# 只看全局符号
nm -g ixgbe.ko

# 动态符号表（.ko 一般没有 .dynsym，用 .symtab）
nm ixgbe.ko | grep ' U ' | wc -l   # 统计依赖多少个外部符号
```

**符号类型速记：**

| 标记 | 含义 | 模块场景 |
|------|------|----------|
| `T` | 代码段全局函数 | 模块导出的驱动接口 |
| `t` | 代码段局部函数 | static 函数 |
| `D` | 已初始化全局数据 | 全局变量 |
| `B` | BSS 段未初始化数据 | 全局变量 |
| `U` | 未定义符号 | **依赖内核提供的函数** |

> **HFT 排错：** 内核模块加载报 `undefined symbol` → `nm -u mod.ko` 列出所有未定义符号 → 逐个在 `vmlinux` 中 `nm vmlinux | grep <symbol>` 确认内核是否导出。详见 C 笔记 `02-Pointers-on-C/ch18/18.5-nm符号表查看.md`。

---

### 内核模块的符号导出

模块通过 `EXPORT_SYMBOL()` 将函数/变量暴露给其他模块使用：

```c
/* my_driver.c */
static int my_send(struct sk_buff *skb, struct net_device *dev) {
    /* ... */
}

/* 导出符号，其他模块可以调用 */
EXPORT_SYMBOL(my_send);
EXPORT_SYMBOL_GPL(my_send);  /* 仅 GPL 许可模块可用 */
```

```bash
# 加载后查看内核符号表
cat /proc/kallsyms | grep my_send
# ffffffffc0a12340 t my_send  [my_driver]

# 模块间依赖关系
lsmod | head
# Module                  Size  Used by
# my_driver              32768  1          ← 被 1 个模块引用
# ixgbe                 200704  0
```

**符号导出 vs 静态链接：**

| 对比 | 内核模块 | 用户态 .so |
|------|----------|-----------|
| 导出方式 | `EXPORT_SYMBOL()` | 默认全局可见 |
| 查找方式 | `cat /proc/kallsyms` | `nm -D libxxx.so` |
| 链接时机 | `insmod` 时动态解析 | `dlopen`/启动时 |
| 符号地址 | 运行时确定（KASLR） | 运行时确定（ASLR） |

---

### 模块加载流程（insmod internals）

```
用户空间                    内核空间
────────                    ────────
insmod ixgbe.ko
    │
    ├─→ sys_init_module()    ─→  load_module() {
    │                           ① ELF 验证（magic/class/type）
    │                           ② 读 .modinfo 检查 license
    │                           ③ 分配模块内存（module_alloc）
    │                           ④ 搬移 .text/.data/.bss 到目标地址
    │                           ⑤ 符号解析：
    │                              遍历 .symtab 的 U 符号
    │                              → 在内核符号表查找地址
    │                              → 填入重定位表
    │                           ⑥ 重定位（见下文）
    │                           ⑦ 注册：加入全局模块链表
    │                           ⑧ 调用 module_init()
    │                         }
    └─→ 返回 0（成功）/ -ENOENT（符号未找到）等
```

**关键步骤：符号解析 + 重定位**

模块 .ko 是 `ET_REL` 类型，代码里的地址都是 **相对地址**，加载到内核的某个地址后需要 **修正**：

```bash
# 查看重定位表
readelf -r ixgbe.ko | head -20

# Relocation section '.rela.text' at offset 0x1234:
# Offset   Info   Type          Sym.Value  Sym. Name + Addend
# 00000001 000002 R_X86_64_PC32 00000000   printk - 4
# 00000010 000005 R_X86_64_PC32 00000000   __netdev_alloc_skb - 4
```

| 字段 | 含义 |
|------|------|
| Offset | .text 中需要修正的位置 |
| Type | 重定位类型（x86_64: R_X86_64_PC32 = 相对调用） |
| Sym.Name | 要查找的符号名 |
| Addend | 偏移调整 |

> 内核加载模块时，对每个重定位项：查 `find_symbol()` 得到符号的实际地址 → 计算最终地址 → 写入 `.text` 的对应 offset。这就是 `.ko` 的灵魂——**加载时修补地址**。详见 C 笔记 `02-Pointers-on-C/ch18/18.6-readelf-ELF结构全景.md` 重定位段部分。

---

### 模块参数

模块可以接受参数，通过 `/sys/module/<name>/parameters/` 暴露：

```c
/* ixgbe_main.c */
static unsigned int num_queues = 1;
module_param(num_queues, uint, 0644);
MODULE_PARM_DESC(num_queues, "Number of queues");

static int debug = 0;
module_param(debug, int, 0644);
```

```bash
# 加载时指定参数
modprobe ixgbe num_queues=4 debug=1

# 运行时查看/修改
cat /sys/module/ixgbe/parameters/num_queues    # 4
echo 8 > /sys/module/ixgbe/parameters/debug    # 改为 8（如果权限允许）
```

> **HFT 调参：** 网卡驱动参数（队列数、中断亲和性、RSS 配置）通过模块参数控制，不需要改代码重新编译。

---

### 模块卸载

```c
/* 模块退出函数 */
static void __exit my_driver_exit(void) {
    unregister_netdev(my_dev);
    free_irq(my_dev->irq, my_dev);
    /* 清理所有资源 */
}

module_exit(my_driver_exit);
```

**卸载条件：**

| 条件 | 检查方式 |
|------|----------|
| 引用计数 = 0 | `lsmod` 的 "Used by" 列 |
| 没有活跃资源 | 驱动 exit 函数需正确释放 |
| 没有其他模块依赖 | `lsmod` 显示依赖关系 |

```bash
# 强制卸载（危险！）
rmmod -f ixgbe    # 不检查引用计数，可能导致 panic

# 安全卸载
rmmod ixgbe       # 等待引用计数归零
```

> **HFT 注意：** 生产环境卸载网卡驱动模块会导致**所有连接断开**。必须先迁移流量（bonding/failover），再卸载。模块 exit 函数必须释放所有资源（IRQ、DMA、内存），否则内存泄漏最终导致系统不稳定。

---

### HFT 定制驱动模块实践

```bash
# 1. 获取驱动源码
git clone https://github.com/intel/ethernet-linux-ixgbe
cd ixgbe-5.20/src/

# 2. 修改源码（例如：定制中断亲和性、NAPI 轮询权重）

# 3. 编译
make

# 4. 验证模块
modinfo ixgbe.ko                    # 检查版本/参数
nm -u ixgbe.ko | grep -v ' U '      # 确认没有意外的未定义符号
readelf -h ixgbe.ko | grep Type     # 确认是 REL 类型

# 5. 加载测试（开发机）
rmmod ixgbe 2>/dev/null
insmod ixgbe.ko num_queues=4

# 6. 验证功能
ip link set eth0 up
ethtool -i eth0                     # 确认驱动版本
cat /proc/interrupts | grep eth0    # 确认中断分配

# 7. 性能测试
iperf3 -c <server> -t 60            # 带宽
dpdk-test                           # DPDK 兼容性
```

---

### 模块 vs 内核内置

| 对比 | 模块 (.ko) | 内核内置 (built-in) |
|------|-----------|-------------------|
| 加载时机 | 运行时 insmod/modprobe | 启动时直接可用 |
| 内存 | 不用时可以卸载释放 | 始终占用内存 |
| 启动依赖 | 需要文件系统加载后才能 insmod | 最早可用 |
| 调试 | 可以单独编译/加载/卸载测试 | 需要重启整个内核 |
| HFT 选择 | 网卡驱动用模块（灵活替换） | 关键子系统内置（如调度器） |

> **HFT 生产实践：** 网卡驱动编译为模块（`=m`），文件系统/存储驱动内置（`=y`）。根文件系统驱动如果编为模块，需要 initramfs 预加载。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** insmod 和 modprobe 的区别？内核模块如何符号导出？

<details><summary>答案</summary>

insmod：直接加载单个 .ko 文件，不处理依赖。modprobe：自动解析依赖（读 modules.dep），按顺序加载依赖模块。模块用 `EXPORT_SYMBOL(symbol)` 导出符号到内核符号表，其他模块可调用。`EXPORT_SYMBOL_GPL` 仅 GPL 模块可用。HFT 定制驱动编译为 .ko，modprobe 加载，可运行时更新驱动不需重启。

</details>

**Q2.** 内核模块 .ko 是什么类型的 ELF 文件？为什么需要重定位？

<details><summary>答案</summary>

.ko 是 `ET_REL`（可重定位文件），和 .o 类似。代码中的地址是相对地址，加载到内核的某个地址后需要修正。内核加载时遍历重定位表（`.rela.text` 等），对每个重定位项：查找符号实际地址 → 计算最终地址 → 写入 .text 对应位置。这就是为什么 `readelf -r mod.ko` 能看到大量重定位项——模块加载时由内核逐个修补。

</details>

**Q3.** 内核模块加载报 `undefined symbol`，怎么排查？

<details><summary>答案</summary>

① `nm -u mod.ko` 列出所有未定义符号（U 标记）。② 逐个在内核符号表中确认：`nm vmlinux | grep <symbol>` 或 `cat /proc/kallsyms | grep <symbol>`。③ 如果内核没有导出该符号，可能是：内核版本不匹配（符号被重命名/删除）、CONFIG 选项未开启（如 CONFIG_NET_RX_BUSY_POLL）、或模块源码版本与内核不匹配。④ 确认模块的 vermagic 与内核版本匹配：`modinfo mod.ko | grep vermagic`。

</details>

**Q4.** HFT 定制网卡驱动为什么用模块而不是内核内置？

<details><summary>答案</summary>

① 灵活替换：`rmmod && insmod` 可以不停机更新驱动，内置需要重启。② 调试方便：单独编译加载，修改后快速验证。③ 生产回滚：出问题可以立即 `rmmod` 换回官方驱动。④ 参数调整：通过 `/sys/module/<name>/parameters/` 运行时调参。⑤ 不污染主线内核：定制代码隔离在 .ko 中，不影响内核稳定性。

</details>

</details>
---
