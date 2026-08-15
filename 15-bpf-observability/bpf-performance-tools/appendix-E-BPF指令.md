# 附录 E BPF 指令

> 底本：《BPF之巅》附录 E（印刷 p812–816）。所选 BPF 指令的总结，帮助从跟踪工具和附录 D hello_world.c 的源码中**读懂指令清单**。不建议直接用指令从头开发 BPF 跟踪程序；完整参考见 Linux 源码头文件：
> 经典 BPF：`include/uapi/linux/filter.h`；扩展 BPF：`include/uapi/linux/bpf.h`；共用编码：`include/uapi/linux/bpf_common.h`

## 辅助宏（表 E-1）

附录 D 的 hello_world.c 中出现的指令即由这些高级宏构成：

| 指令宏 | 描述 |
|---|---|
| `BPF_ALU64_REG(OP, DST, SRC)` | ALU 64 位寄存器操作 |
| `BPF_ALU32_REG(OP, DST, SRC)` | ALU 32 位寄存器操作 |
| `BPF_ALU64_IMM(OP, DST, IMM)` | ALU 64 位立即数运算 |
| `BPF_ALU32_IMM(OP, DST, IMM)` | ALU 32 位立即数运算 |
| `BPF_MOV64_REG(DST, SRC)` | 64 位寄存器间移动 |
| `BPF_MOV32_REG(DST, SRC)` | 32 位寄存器间移动 |
| `BPF_MOV64_IMM(DST, IMM)` | 64 位立即数移动到目标 |
| `BPF_MOV32_IMM(DST, IMM)` | 32 位立即数移动到目标 |
| `BPF_LD_IMM64(DST, IMM)` | 加载 64 位立即数 |
| `BPF_LD_MAP_FD(DST, MAP_FD)` | 将映射 fd 加载到寄存器 |
| `BPF_LDX_MEM(SIZE, DST, SRC, OFF)` | 从内存加载到寄存器 |
| `BPF_STX_MEM(SIZE, DST, SRC, OFF)` | 从寄存器存储到内存 |
| `BPF_STX_XADD(SIZE, DST, SRC, OFF)` | 寄存器原子内存加（XADD） |
| `BPF_ST_MEM(SIZE, DST, OFF, IMM)` | 从立即数存储到内存 |
| `BPF_JMP_REG(OP, DST, SRC, OFF)` | 比较寄存器条件跳转 |
| `BPF_JMP_IMM(OP, DST, IMM, OFF)` | 比较立即数条件跳转 |
| `BPF_JMP32_REG(OP, DST, SRC, OFF)` | 32 位比较寄存器 |
| `BPF_JMP32_IMM(OP, DST, IMM, OFF)` | 32 位比较立即数 |
| `BPF_JMP_A(OFF)` | 无条件跳转 |
| `BPF_LD_MAP_VALUE(DST, MAP_FD, OFF)` | 将映射值指针加载到寄存器 |
| `BPF_CALL_REL(IMM)` | 相对调用（BPF 到 BPF） |
| `BPF_EMIT_CALL(FUNC)` | 辅助函数调用 |
| `BPF_RAW_INSN(CODE, DST, SRC, OFF, IMM)` | 原始 BPF 代码 |
| `BPF_EXIT_INSN()` | 退出 |

（BPF_LD_ABS 和 BPF_LD_IND 已弃用，未收录。）

**缩写字典**（按字母序）：32/64 = 位宽；ALU = 算术逻辑单元；DST = 目的；FUNC = 函数；IMM = 立即数（代码中提供的常量）；INSN = 指令；JMP = 跳转；LD = 加载；LDX = 从寄存器加载（load extended）；MAP_FD = 映射文件描述符；MEM = 内存；MOV = 移动；OFF = 偏移量；OP = 操作；REG = 寄存器；REL = 相对；ST = 存储；SRC = 源；STX = 存储到寄存器（store extended）。

## 指令（表 E-2）

指令通常由**指令类 + 按位或组合的字段**构成：

| 类型 | 名称 | 编号 | 描述 |
|---|---|---|---|
| 指令类（经典） | BPF_LD | 0x00 | 加载 |
| 指令类（经典） | BPF_LDX | 0x01 | 加载到 X |
| 指令类（经典） | BPF_ST | 0x02 | 存储 |
| 指令类（经典） | BPF_STX | 0x03 | 存储到 X |
| 指令类（经典） | BPF_ALU | 0x04 | 算术逻辑单元 |
| 指令类（经典） | BPF_JMP | 0x05 | 跳转 |
| 指令类（经典） | BPF_RET | 0x06 | 返回 |
| 指令类（扩展） | BPF_ALU64 | 0x07 | ALU 64 位 |
| 大小（经典） | BPF_W | 0x00 | 32 位字 |
| 大小（经典） | BPF_H | 0x08 | 16 位半字 |
| 大小（经典） | BPF_B | 0x10 | 8 位字节 |
| 大小（扩展） | BPF_DW | 0x18 | 64 位双字 |
| 存储修饰符（扩展） | BPF_XADD | 0xc0 | 排他添加 |
| ALU/跳转操作（经典） | BPF_ADD | 0x00 | 加法 |
| ALU/跳转操作（经典） | BPF_SUB | 0x10 | 减法 |
| ALU/跳转操作（经典） | BPF_K | 0x00 | 立即数操作数 |
| ALU/跳转操作（经典） | BPF_X | 0x08 | 寄存器操作数 |
| ALU/跳转操作（扩展） | BPF_MOV | 0xb0 | 寄存器间移动 |
| 跳转操作（扩展） | BPF_JLT | 0xa0 | 无符号小于比较后跳转 |
| 寄存器编号（扩展） | BPF_REG_0 ~ BPF_REG_10 | 0x00 ~ 0x0a | 0 号 ~ 10 号寄存器 |

## 编码（表 E-3）

扩展 BPF 指令格式（`struct bpf_insn`），单条指令 8 字节：

| 操作码 | 目标寄存器 | 源寄存器 | 有符号偏移量 | 有符号立即数 |
|---|---|---|---|---|
| 8 位 | 8 位 | 8 位 | 16 位 | 32 位 |

**展开示例**：hello_world.c 第一条指令

```c
BPF_MOV64_IMM(BPF_REG_1, 0xa21)
```

操作码展开为 `BPF_ALU64 | BPF_MOV | BPF_K`，查表 E-3/E-2 得 **0xb7**；参数部分设目标寄存器 BPF_REG_1（0x01）和立即数 0xa21。

## 用 bpftool 验证字节码

```bash
# bpftool prog
907: kprobe  tag 9abf0e9561523153
    loaded at 2019-01-08T23:22:00+0000  uid 0
    xlated 128B  jited 117B  memlock 4096B
# bpftool prog dump xlated id 907 opcodes
   0: (b7) r1 = 2593
      b7 01 00 00 21 0a 00 00
   1: (6b) *(u16 *)(r10 - 4) = r1
      6b 1a fc ff 00 00 00 00
   2: (b7) r1 = 1684828783
      b7 01 00 00 6f 72 6c 64
   3: (63) *(u32 *)(r10 - 8) = r1
      63 1a f8 ff 00 00 00 00
```

`0: (b7) r1 = 2593` 即上面的 BPF_MOV64_IMM(BPF_REG_1, 0xa21)（2593 = 0xa21）。

**对跟踪工具的意义**：许多 BPF 指令用于从结构体加载数据，然后调 BPF 辅助函数把值存进映射或发出 perf 记录（见 2.3.6 节"BPF 辅助函数"）。

## 参考资料

- `Documentation/networking/filter.txt`
- `include/uapi/linux/bpf.h`
- Cilium《BPF and XDP Reference Guide》

## 相关章节

- 上一章：[appendix-D-C语言BPF.md](./appendix-D-C语言BPF.md)
