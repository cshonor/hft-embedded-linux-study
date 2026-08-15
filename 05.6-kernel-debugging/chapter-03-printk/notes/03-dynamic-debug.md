# 3.3 dynamic debug 框架

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

Dynamic Debug (dyndbg) 是内核运行时调试的核心——`pr_debug()` 编译进内核但默认不输出，通过 dyndbg 按需开关，不需要重编译。

## dyndbg 工作原理

```
pr_debug("message") 编译为:

// CONFIG_DYNAMIC_DEBUG=y 时:
static struct _ddebug descriptor = {
    .format = "message",
    .function = __func__,
    .filename = __FILE__,
    .lineno = __LINE__,
    .flags = 0,  // 默认不打印
};
if (unlikely(descriptor.flags & _DPRINTK_FLAGS_PRINT))
    printk(KERN_DEBUG "message");

// CONFIG_DYNAMIC_DEBUG=n, DEBUG 未定义时: 编译为空语句
// CONFIG_DYNAMIC_DEBUG=n, DEBUG 定义时: printk(KERN_DEBUG "message")
```

## 使用方法

### 基本命令

```bash
# 1. 查看所有可控制的 debug 消息
cat /sys/kernel/debug/dynamic_debug/control | head -20
# 格式: filename:line [module]function "format" flags

# 示例输出:
# drivers/net/eth.c:123 [net]eth_open "opened interface" =p
# drivers/net/eth.c:456 [net]eth_rx "received packet, len=%d" _

# 2. 按文件名启用
echo 'file my_driver.c +p' > /sys/kernel/debug/dynamic_debug/control

# 3. 按模块名启用
echo 'module my_module +p' > /sys/kernel/debug/dynamic_debug/control

# 4. 按函数名启用
echo 'func my_probe +p' > /sys/kernel/debug/dynamic_debug/control

# 5. 按格式字符串匹配
echo 'format "tx timeout" +p' > /sys/kernel/debug/dynamic_debug/control

# 6. 启用所有
echo '+p' > /sys/kernel/debug/dynamic_debug/control

# 7. 禁用
echo 'file my_driver.c -p' > /sys/kernel/debug/dynamic_debug/control

# 8. 启用并附加额外信息
echo 'file my_driver.c +pmlt' > /sys/kernel/debug/dynamic_debug/control
```

### 标志说明

| 标志 | 含义 | 示例输出 |
|------|------|---------|
| `p` | print（启用输出） | `[  12.345] my_module: message` |
| `f` | 添加函数名 | `[  12.345] my_module my_probe: message` |
| `l` | 添加行号 | `[  12.345] my_module:123 message` |
| `m` | 添加模块名 | `[  12.345] my_module: message` |
| `t` | 添加时间戳 | `[  12.345678] message` |

### 过滤组合

```bash
# 多条件 AND（空格分隔）
echo 'module my_module file my_driver.c +p' > control

# 排除特定函数
echo 'module my_module func !unimportant_* +p' > control

# 通配符
echo 'file drivers/net/* +p' > control
echo 'file drivers/net/eth*.c +p' > control

# 行号范围
echo 'file my_driver.c line 100-200 +p' > control
```

## 内核模块中使用

```c
// 在模块中使用 pr_debug
pr_debug("probe called for device %s\n", dev_name(dev));

// 加载模块时启用
modprobe my_module dyndbg="+p"

# 或在 modprobe.conf 中持久化:
# /etc/modprobe.d/my_module.conf
# options my_module dyndbg="+p"

// 设备相关
dev_dbg(dev, "register read: 0x%x\n", reg_val);

// 启动时通过内核命令行
// kernel cmdline: my_module.dyndbg=+p
// 或全局: dyndbg="file my_driver.c +p; module my_module +pmlt"
```

## 高级用法

### dyndbg 脚本

```bash
#!/bin/bash
# debug-hft-driver.sh: 启用 HFT 驱动调试

CONTROL=/sys/kernel/debug/dynamic_debug/control

# 启用所有 HFT 相关模块
echo "module hft_driver +pmlt" > $CONTROL
echo "module hft_net +pmlt" > $CONTROL

# 启用特定函数
echo "func hft_process_packet +pmlt" > $CONTROL
echo "func hft_dma_callback +pmlt" > $CONTROL

# 查看当前状态
echo "=== Active debug messages ==="
grep "=p" $CONTROL | grep hft

# 监控输出
echo "=== Monitoring (Ctrl-C to stop) ==="
dmesg -w | grep hft
```

### 运行时切换

```bash
# 在性能敏感操作前关闭
echo 'module hft_driver -p' > control
# 执行性能敏感操作
./run_benchmark
# 操作后重新开启
echo 'module hft_driver +p' > control
```

## pr_debug 运行时开销

```c
// CONFIG_DYNAMIC_DEBUG=y 时 pr_debug 的编译结果:
// 一次内存读取 + 条件判断（flags & _DPRINTK_FLAGS_PRINT）
// 未启用时开销: ~1-2ns（分支预测成功）
// 启用时开销: 格式化 + printk（~1-10μs）

// 对比 printk(KERN_DEBUG):
// 始终执行格式化和写入 ring buffer
// 即使控制台不输出，仍有 ring buffer 写入开销
```

## HFT 关联

dyndbg 是 HFT 内核模块调试的**核心工具**——不需要重编译，生产环境按需开关。

HFT 使用策略：
1. **代码中大量使用 `pr_debug()`**：热路径、错误处理、状态变化
2. **平时关闭**：生产环境不输出，零开销
3. **出问题时精确启用**：按文件/函数/模块开关
4. **附加时间戳**：`+pt` 测量时序
5. **持久化配置**：写入 `/etc/modprobe.d/`

```c
// HFT 驱动中的 pr_debug 使用规范
static int hft_process_packet(struct hft_dev *dev, struct sk_buff *skb)
{
    pr_debug("enter: skb=%p len=%u\n", skb, skb->len);

    // 热路径详细追踪
    pr_debug("rx: seq=%u ts=%llu price=%d\n",
             hdr->seq, hdr->timestamp, hdr->price);

    if (unlikely(hdr->seq != dev->expected_seq)) {
        pr_debug("seq mismatch: expected=%u got=%u\n",
                 dev->expected_seq, hdr->seq);
        // ...
    }

    pr_debug("exit: ret=%d\n", ret);
    return ret;
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `pr_debug()` 在未启用 dyndbg 时有运行时开销吗？

> 有少量开销。`pr_debug()` 编译为对 `_ddebug` 描述符的检查——判断 flags 中 `p` 位是否设置。如果未设置则直接返回，开销很小（一次内存读取 + 条件判断，约 1-2ns）。实际格式化和输出只在启用时发生。

**Q2:** 如何在内核启动时就启用某个驱动的 dyndbg？

> 通过内核命令行参数：`my_module.dyndbg=+p` 或 `ddebug_query="file my_driver.c +p"`。也可以在 `/etc/modprobe.d/my_module.conf` 中设置 `options my_module dyndbg=+p`。

**Q3:** Dynamic Debug 的 +p 和 _p flag 有什么区别？

> +p 启用打印，-p 禁用。_p 是"如果当前已启用则保持启用"，用于条件追加。例如 `echo "file:mm/slub.c +p" > control` 启用 slub.c 中所有 pr_debug。`echo "file:mm/slub.c _p" > control` 只在已启用的情况下保持。

**Q4:** dev_dbg() 和 pr_debug() 有什么区别？什么时候用哪个？

> dev_dbg() 关联到 device 结构体，输出包含设备名（如 "my_device: message"）。pr_debug() 无设备关联。驱动代码中用 dev_dbg()，通用内核代码用 pr_debug()。两者都受 Dynamic Debug 控制。

**Q5:** 如何只启用特定行号范围的 pr_debug？

> 使用 `line` 过滤器：`echo 'file my_driver.c line 100-200 +p' > control`。这只会启用 my_driver.c 第 100-200 行的 pr_debug 调用。适合在精确定位后只输出关键区域的调试信息，避免日志过多。

</details>

## 交叉引用

- [05.6 ch03 printk 基础](../../chapter-03-printk/notes/01-printk-basics-loglevel.md)
- [05.6 ch03 dev_dbg](../../chapter-03-printk/notes/04-dev-dbg.md)
- [05.6 ch03 trace_printk](../../chapter-03-printk/notes/05-ftrace-printk.md)
