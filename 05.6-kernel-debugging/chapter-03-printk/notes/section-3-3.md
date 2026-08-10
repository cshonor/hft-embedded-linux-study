# 3.3 dynamic debug 框架

> 🔴 精读

## 本节要点

### dynamic debug (dyndbg) 是什么

6.x 内核中 `pr_debug()` / `dev_dbg()` 默认编译进内核但**不输出**，通过 dyndbg 框架在运行时动态开关。

### 使用方法

```bash
# 1. 查看所有可控制的 debug 消息
cat /sys/kernel/debug/dynamic_debug/control | head -20
# 格式: filename:line [module]function "format" flags

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

# 8. 启用并附加行号/模块名前缀
echo 'file my_driver.c +pml' > /sys/kernel/debug/dynamic_debug/control
# p=print, m=module, l=line, t=timestamp, f=function
```

### 标志说明

| 标志 | 含义 |
|------|------|
| `p` | print（启用输出） |
| `f` | 在消息中添加函数名 |
| `l` | 添加行号 |
| `m` | 添加模块名 |
| `t` | 添加时间戳 |

### 内核模块中使用

```c
// 在模块中使用 pr_debug
pr_debug("probe called for device %s\n", dev_name(dev));

// 加载模块时启用
modprobe my_module dyndbg="+p"
# 或在 modprobe.conf 中:
# options my_module dyndbg=+p

// 设备相关
dev_dbg(dev, "register read: 0x%x\n", reg_val);
```

### HFT 关联

dyndbg 是 HFT 内核模块调试的**核心工具**——不需要重编译，生产环境按需开关。交易热路径用 `pr_debug()`，平时关闭，出问题时精确启用特定函数。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `pr_debug()` 在未启用 dyndbg 时有运行时开销吗？

> 有少量开销。`pr_debug()` 编译为对 `_ddebug` 描述符的检查——判断 flags 中 `p` 位是否设置。如果未设置则直接返回，开销很小（一次内存读取 + 条件判断）。实际格式化和输出只在启用时发生。

**Q2:** 如何在内核启动时就启用某个驱动的 dyndbg？

> 通过内核命令行参数：`my_module.dyndbg=+p` 或 `ddebug_query="file my_driver.c +p"`。也可以在 `/etc/modprobe.d/my_module.conf` 中设置 `options my_module dyndbg=+p`。


**Q:** Dynamic Debug 的 +p 和 _p flag 有什么区别？

> +p 启用打印，-p 禁用。_p 是"如果当前已启用则保持启用"，用于条件追加。例如 `echo "file:mm/slub.c +p" > control` 启用 slub.c 中所有 pr_debug。`echo "file:mm/slub.c _p" > control` 只在已启用的情况下保持。

**Q:** dev_dbg() 和 pr_debug() 有什么区别？什么时候用哪个？

> dev_dbg() 关联到 device 结构体，输出包含设备名（如 "my_device: message"）。pr_debug() 无设备关联。驱动代码中用 dev_dbg()，通用内核代码用 pr_debug()。两者都受 Dynamic Debug 控制。

</details>

## 交叉引用

- [05.6 ch03 dev_dbg](chapter-03-printk/notes/section-3-4.md)
