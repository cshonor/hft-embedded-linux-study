#ifndef P1_CPU_H
#define P1_CPU_H

#include "alu.h"
#include "isa.h"

#include <stdint.h>

/*
 * 控制器是一个 FSM（有限状态机，笔记 00 ch3.4）。
 * 真 CPU 用触发器记住「现在处于哪一拍」；这里用 enum。
 *
 * 一条指令拆成 4 拍（HALT 在译码拍就停）：
 *   FETCH     按 PC 从 IMem 取出 16 位指令，PC+1
 *   DECODE    切开 opcode / 寄存器编号 / 立即数
 *   EXECUTE   ALU 运算、访存、或改 PC（跳转）
 *   WRITEBACK 若需要，把结果写入 rd
 *
 * 笔记 7.3 讲的是「一拍做完」的单周期；本项目故意多周期，才能 --trace 看见每一拍。
 */
enum CpuState {
    ST_FETCH = 0,
    ST_DECODE,
    ST_EXECUTE,
    ST_WRITEBACK,
    ST_HALT
};

struct Cpu {
    uint8_t r[4];          /* 通用寄存器 R0..R3 */
    uint8_t pc;            /* 程序计数器：下一条指令在 IMem 的下标 */
    uint16_t ir;           /* 指令寄存器：当前正在执行的那条 16 位码 */
    struct AluFlags flags; /* Z C N V，JNZ 读的是这里 */
    enum CpuState state;   /* FSM 现态 */
    int halted;            /* 1 = 已停，不再走时钟 */
    int cycles;            /* 已经过了多少拍（不是「执行了几条指令」） */
    int trace;             /* 1 = 每拍打印一行，给新手看流水 */

    uint16_t imem[256];    /* 指令存储器，哈佛结构：代码和数据分开 */
    uint8_t dmem[256];     /* 数据存储器，LOAD/STORE 走这里 */

    /* 译码后锁存的字段：相当于「指令拆开后的导线」 */
    int op, rd, rs, rt;
    uint8_t imm;
    int reg_we;            /* 写使能：本拍结束要不要写寄存器堆 */
    uint8_t wb_val;        /* 准备写回 rd 的值 */
};

void cpu_reset(struct Cpu *c);

/* 前进 1 个时钟沿：只做当前 state 该做的事，然后切到下一状态。 */
void cpu_clock(struct Cpu *c);

/* 一直打拍直到 HALT，或超过 max_cycles（防止死循环）。返回是否正常停机。 */
int cpu_run(struct Cpu *c, int max_cycles);

void load_sum(struct Cpu *c);
void load_fib(struct Cpu *c);
void load_memcpy(struct Cpu *c);

#endif
