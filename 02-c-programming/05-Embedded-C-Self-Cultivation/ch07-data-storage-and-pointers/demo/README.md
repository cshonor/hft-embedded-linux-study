# ch07 Demo

```bash
make all

./demo01_array_ptr
gdb -batch -ex run -ex 'print sizeof(buf)' -ex 'print sizeof(p)' -ex 'x/16xb buf' ./demo01_array_ptr

./demo02_double_ptr
./demo03_struct_offset
./demo04_func_ptr

# volatile 汇编对比（建议 -O2 更明显）
make demo05_volatile CFLAGS="-g -O2 -Wall -std=gnu11"
objdump -d demo05_volatile | grep -A8 poll_volatile
objdump -d demo05_volatile | grep -A8 poll_normal

./demo06_dangling
# 取消 demo06 内注释后: gdb ./demo06_dangling

./demo07_mmio
```

## demo04 函数指针跳转表

四则运算 `jump_table[OP_*]`，衔接 **7.12** 与 **ch08 OOP**。

## demo05 说明

| 变量 | 行为 |
|------|------|
| `volatile int irq_flag` | 循环内每次从内存 reload |
| 普通 `int` | `-O2` 可能被优化成死循环或常量 |

```bash
gcc -S -O2 -o v.s demo05_volatile.c
```

## demo07 MMIO

用户态 `fake_uart` 模拟 `volatile` 结构体寄存器映射；真板子替换为固定物理地址指针。
