# ch08 Demo

```bash
make all

./demo01_encapsulation
./demo02_inherit
./demo03_polymorphism
valgrind --leak-check=full ./demo04_lifecycle
./demo05_multi_iface
./demo06_layered
```

## demo03 多态 UART/SPI

`dev_ops.c` 定义 `uart_ops` / `spi_ops` 虚表；`app_send()` 仅依赖 `struct dev_obj *`，运行时绑定不同硬件实现。

```bash
gdb ./demo02_inherit
(gdb) print dev->open
(gdb) print &uart
```

## demo06 三层目录

```
layered/
  abstract/   device.h device.c  — 抽象接口
  hw/         uart.c              — 硬件适配
  app/        app.c main.c        — 业务层
```

业务层只 `#include` 抽象头文件，不直接依赖 `hw/uart.c` 内部实现。

## valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./demo03_polymorphism
valgrind --leak-check=full ./demo04_lifecycle
```
