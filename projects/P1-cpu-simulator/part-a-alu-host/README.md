# P1 part-a — 8-bit ALU（主机 C）

> 卡住翻：[5.2 算术电路](../../../00-digital-logic-cpu/ch05_digital_blocks/5.2_算术电路.md) · [2.8.3 MUX](../../../00-digital-logic-cpu/ch02_combinational/2.8.3_MUX.md)

Logisim 仍是「看见门」的推荐路径。这台机器没有图形化电路环境，先用 C 把 **加减与或移位 + Z/C/N/V** 钉死，供 part-b 多周期 CPU 调用。

```bash
make test
```

Windows 用 WSL：

```bash
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/12392/Desktop/hft/projects/P1-cpu-simulator/part-a-alu-host && make test'
```

| 标志 | 含义 |
|------|------|
| Z | 结果为 0 |
| N | 结果 bit7 |
| C | 加法进位 / 减法借位 |
| V | 有符号溢出 |

下一站：[part-b-multicycle](../part-b-multicycle/)
