# ch06 Demo

```bash
make all

# packed / aligned
./demo01_packed_struct
gdb -batch -ex 'break main' -ex run -ex 'x/16xb &p' ./demo01_packed_struct

# 自定义 ELF 段
readelf -S demo02_custom_section | grep -E 'my_|Name'
nm demo02_custom_section | grep g_custom

# weak 弱符号
./demo03_weak              # 输出 weak 默认
./demo03_weak_override     # 输出 strong 覆盖

# constructor / destructor
./demo04_constructor

# 柔性数组
./demo05_flexible_array

# 日志宏 + 语句表达式 MAX
./demo06_log_macro

# 内嵌汇编 / 寄存器约束
./demo07_reg_asm
```

## 工具验证

```bash
size demo01_packed_struct demo02_custom_section
objdump -t demo02_custom_section | grep my_
gcc -Wall -Wformat -o fmt_test demo06_log_macro.c   # format 属性见 6.8
```

## demo03 说明

| 目标 | 链接 | 行为 |
|------|------|------|
| `demo03_weak` | main + hw_weak.c | 调用 weak 默认 stub |
| `demo03_weak_override` | main + weak + hw_strong.c | 强符号覆盖 weak |

## demo07 ARM 交叉（可选）

```bash
# 见 ch03 交叉工具链
# register int val asm("r0") = 1;
# asm volatile("mov %0, #0" : "=r"(val));
```
