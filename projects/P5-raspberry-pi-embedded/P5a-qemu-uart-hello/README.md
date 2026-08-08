# P5a — QEMU 裸机 UART Hello World

> 在 QEMU 模拟的 ARM 机器上，不依赖任何 OS，直接写汇编点亮 UART 打印 "Hello"。

## 项目目标

理解"裸机"——CPU 上电后第一条指令在哪、怎么跑到你的代码、怎么不用 printf 把字节送出串口。建立 ARM 异常等级、启动地址、MMU 关闭状态下的直访外设直觉。

## 交付物

- [ ] AArch64 汇编启动文件（设栈、跳 main）
- [ ] UART 寄存器直访（PL011 或 mini UART），轮询发送字节
- [ ] 链接脚本（指定加载地址 `. = 0x80000`）
- [ ] QEMU 启动命令：`qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio`
- [ ] 看到 "Hello, bare metal!" 输出

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`10` arm-architecture](../../../10-arm-architecture/) | AArch64 汇编、异常等级 EL1/EL2、启动流程 |

## 前置

[P4](../../P4-kernel-module/)（理解内核态）。

## 学习目标

- 上电后 PC 指向哪、谁把镜像加载到内存
- 特权级 EL3→EL2→EL1 的降落（或直接 EL1）
- 设备寄存器是内存映射的（MMIO），无 OS 时直接读写物理地址
- 链接脚本如何控制代码加载位置

## 里程碑

1. **M1** 汇编写死一个字符到 UART 寄存器，QEMU 串口看到
2. **M2** 封装 `uart_puts` 函数，C 可调用
3. **M3** 链接脚本 + 启动栈，完整 bare-metal Hello

## 参考模块

- [10-arm-architecture/](../../../10-arm-architecture/) — ARM64 体系结构编程与实践、ARM 汇编
