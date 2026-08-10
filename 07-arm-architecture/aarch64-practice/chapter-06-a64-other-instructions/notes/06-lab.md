# 6.6 实验要点

> 来源：§6.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

通过 QEMU + GDB 实验，验证 ADRP、SVC、MRS/MSR、LDXR/STXR、DMB 等指令的行为。

## 实验列表

| 实验 | 内容 | 平台 | 重点 |
|------|------|------|------|
| 6-1 | ADRP+ADD 获取全局变量地址 | QEMU | PC 相对寻址 |
| 6-2 | SVC 系统调用（EL0→EL1） | QEMU | 异常切换流程 |
| 6-3 | MRS/MSR 读写系统寄存器 | QEMU | 系统寄存器访问权限 |
| 6-4 | LDXR/STXR 原子自增 | QEMU | 独占监视器 |
| 6-5 | DMB 屏障效果验证 | QEMU | 内存序 |

## 实验 6-1：ADRP+ADD 获取全局变量地址

**目标**：理解 PC 相对寻址，对比 ADRP+ADD 和 LDR =伪指令。

```asm
; exp6_1.s
.data
.global counter
counter:
    .quad 0x1234567890ABCDEF

.text
.global _start
_start:
    ; 方法1：ADRP+ADD
    adrp x0, counter
    add  x0, x0, :lo12:counter
    ldr  x1, [x0]           ; 读 counter 值

    ; 方法2：ADR（如果距离够近）
    adr  x2, counter
    ldr  x3, [x2]           ; 应该和 x1 相同

    ; 方法3：LDR =伪指令
    ldr  x4, =counter       ; 文字池加载地址
    ldr  x5, [x4]           ; 应该和 x1 相同

done:
    b done
```

**GDB 验证**：
```
(gdb) b done
(gdb) c
(gdb) p/x $x0     ; ADRP+ADD 的地址
(gdb) p/x $x2     ; ADR 的地址
(gdb) p/x $x4     ; LDR = 的地址
; 三个地址应该相同
(gdb) p/x $x1     ; 应该是 0x1234567890ABCDEF
(gdb) info address counter  ; 对比符号表地址
```

## 实验 6-2：SVC 系统调用

**目标**：跟踪 SVC 执行后的 EL 切换和异常处理流程。

```asm
; exp6_2.s
.text
.global _start
_start:
    ; write(1, msg, 12)
    mov  x8, #64             ; syscall: write
    mov  x0, #1              ; fd: stdout
    adr  x1, msg             ; buffer
    mov  x2, #12             ; length
    svc  #0                  ; 触发系统调用

    ; exit(0)
    mov  x8, #93             ; syscall: exit
    mov  x0, #0              ; status: 0
    svc  #0

msg:
    .ascii "hello world\n"
```

**GDB 验证**：
```
(gdb) b _start
(gdb) c
(gdb) si                    ; 单步到 SVC
(gdb) si                    ; 执行 SVC → 进入异常处理
(gdb) p/x $CurrentEL        ; 应该从 EL0 变为 EL1
(gdb) p/x $ELR_EL1          ; SVC 的返回地址（下一条指令）
(gdb) p/x $ESR_EL1          ; 检查 EC 字段（应为 0x15 = SVC）
(gdb) p/x $x8               ; 系统调用号 64
```

## 实验 6-3：MRS/MSR 读写系统寄存器

**目标**：验证系统寄存器访问权限和 CurrentEL 读取。

```asm
; exp6_3.s（在 EL1 运行）
.text
.global _start
_start:
    ; 读取当前异常等级
    mrs  x0, CurrentEL
    lsr  x0, x0, #2
    and  x0, x0, #3          ; x0 = EL (1)

    ; 读取 SCTLR_EL1
    mrs  x1, SCTLR_EL1       ; 系统控制寄存器

    ; 读取 CNTVCT_EL0（时间戳）
    mrs  x2, CNTVCT_EL0      ; 当前时间戳
    mrs  x3, CNTFRQ_EL0      ; 频率

done:
    b done
```

**GDB 验证**：
```
(gdb) b done
(gdb) c
(gdb) p/x $x0     ; EL 值（裸机通常是 EL1 = 0x1）
(gdb) p/x $x1     ; SCTLR_EL1 值（检查 MMU/Cache 是否开启）
(gdb) p/x $x2     ; 时间戳
(gdb) p/x $x3     ; 频率（如 0x3B9ACA00 = 1GHz, 或 0x3BACA00 = 62.5MHz）
```

**权限验证**（在 EL0 执行 MRS SCTLR_EL1）：
```
; 在 EL0 运行以下代码会触发异常
; mrs x0, SCTLR_EL1  → 同步异常 → SIGILL
```

## 实验 6-4：LDXR/STXR 原子自增

**目标**：验证独占监视器的行为和 STXR 返回值。

```asm
; exp6_4.s
.data
.global counter
counter:
    .quad 0

.text
.global _start
_start:
    adr  x1, counter
    mov  x0, #100            ; 自增 100 次
loop:
    ldxr x2, [x1]            ; 独占加载
    add  x2, x2, #1          ; 自增
    stxr w3, x2, [x1]        ; 独占存储
    cbnz w3, loop            ; 失败则重试
    subs x0, x0, #1
    bne  loop

done:
    b done
```

**GDB 验证**：
```
(gdb) b loop
(gdb) c
(gdb) si                  ; LDXR
(gdb) p/x $x2             ; 当前 counter 值
(gdb) si                  ; ADD
(gdb) p/x $x2             ; +1 后的值
(gdb) si                  ; STXR
(gdb) p/x $w3             ; 返回值（0=成功，非0=失败）
(gdb) si                  ; CBNZ
; 单核环境下 w3 应该总是 0（没有竞争）
```

**多核竞争测试**：启动 QEMU 的 `-smp 2` 双核模式，两个核同时自增同一计数器，观察 STXR 失败和重试。

## 实验 6-5：DMB 屏障效果验证

**目标**：观察弱序内存模型下的乱序行为和 DMB 的排序效果。

```asm
; exp6_5.s
.data
.global data_buf, flag
data_buf:
    .quad 0
flag:
    .quad 0

.text
; 不加屏障的写序（可能乱序）
no_barrier:
    str  x0, [data_buf]      ; 写数据
    str  x1, [flag]           ; 写标志
    ; CPU 可能重排为：先写 flag 后写 data_buf

; 加 DMB 的写序（保证顺序）
with_barrier:
    str  x0, [data_buf]      ; 写数据
    dmb  ish                  ; 屏障
    str  x1, [flag]           ; 写标志
    ; CPU 保证 data_buf 的写在 flag 之前

; 用 STLR 替代（更优雅）
with_stlr:
    str  x0, [data_buf]      ; 写数据
    stlr x1, [flag]           ; Store-Release：自动排序
```

**验证方法**：
1. 在多核 QEMU 上，核 A 执行写序，核 B 轮询 flag 后读 data_buf
2. 不加屏障时，核 B 可能读到 flag=1 但 data_buf 还是旧值
3. 加 DMB 或用 STLR 后，核 B 读到 flag=1 时 data_buf 一定是新值

## 自测题

1. 实验中如何验证 ADRP+ADD 获取的地址正确？
<details><summary>答案</summary>
1. 用 GDB 打印 ADRP 后的 x0（页基地址）和 ADD 后的 x0（精确地址）
2. 与符号表中的地址对比：`info address global_var`
3. 用获取的地址 LDR 读取值，与预期对比
</details>

2. 如何在实验中观察 LDXR/STXR 的重试？
<details><summary>答案</summary>
1. 在 STXR 后设断点，检查 w2（返回值）
2. 如果 w2 != 0，说明发生了重试
3. 多核场景下用 GDB attach 到另一个核，在 LDXR 和 STXR 之间写同一地址
4. 观察监视器失效导致的 STXR 失败
</details>

3. 实验 6-3 中，在 EL0 运行 `MRS x0, SCTLR_EL1` 会怎样？
<details><summary>答案</summary>
触发同步异常（非法指令使用）。EL0 没有权限访问 EL1 的系统寄存器。异常向量表中的同步异常处理代码会识别这是权限错误，通常向用户进程发送 SIGILL 信号。可以用 GDB 设置 `catch signal SIGILL` 来捕获。
</details>

4. 在实验 6-5 中，如何证明"不加屏障时写操作会乱序"？
<details><summary>答案</summary>
设计双核实验：
- 核 A：写 data_buf，写 flag（不加屏障）
- 核 B：循环读 flag，flag=1 后读 data_buf
- 如果有乱序，核 B 可能读到 flag=1 但 data_buf=0（旧值）
- 多次运行，统计出现 data_buf=0 的次数
- 加 DMB 或 STLR 后，这个问题应该消失

注意：单核上观察不到乱序（单核的乱序对自身是透明的），必须在多核上验证。
</details>

5. 如何在 GDB 中查看 ESR_EL1 的 EC 字段判断异常类型？
<details><summary>答案</summary>
```
(gdb) p/x $ESR_EL1
```
ESR_EL1 的 bit[31:26] 是 EC（Exception Class）。常见值：
- 0x15 → SVC 指令
- 0x21 → 数据中止
- 0x22 → 对齐异常
- 0x24 → 非法指令
- 0x25 → 当前EL的非法指令使用

用 `(gdb) p/x ($ESR_EL1 >> 26)` 提取 EC 字段。
</details>

## 参考与延伸

- 原书 §6.6
- [6.1 ADR/ADRP](01-adr-adrp.md)
- [6.2 SVC](02-svc.md)
- [6.4 LDXR/STXR](04-ldxr-stxr-preview.md)
- [6.5 屏障](05-barrier-preview.md)
