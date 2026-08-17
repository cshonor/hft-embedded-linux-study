#include "cpu.h"

#include <stdio.h>
#include <string.h>

static const char *state_name(enum CpuState s)
{
    switch (s) {
    case ST_FETCH:     return "FETCH";
    case ST_DECODE:    return "DECODE";
    case ST_EXECUTE:   return "EXECUTE";
    case ST_WRITEBACK: return "WB";
    case ST_HALT:      return "HALT";
    }
    return "?";
}

void cpu_reset(struct Cpu *c)
{
    memset(c, 0, sizeof *c); /* 寄存器、内存、标志全清零 */
    c->state = ST_FETCH;     /* 上电后第一拍就是取指 */
}

static void apply_flags(struct Cpu *c, struct AluResult res)
{
    c->flags = res.flags;
}

/*
 * 注意：这一行打印的是「本拍刚开始」时的寄存器。
 * 写回发生在 WB 拍的后半，所以 WB 那一行里的 r[] 还是旧值，
 * 新值要到下一拍 FETCH 才能看见。看 --trace 时别慌。
 */
static void dump(const struct Cpu *c)
{
    if (!c->trace) {
        return;
    }
    printf("cyc=%-4d %-8s pc=%02x ir=%04x r=[%02x %02x %02x %02x] Z=%d C=%d N=%d V=%d\n",
           c->cycles,
           state_name(c->state),
           c->pc,
           c->ir,
           c->r[0], c->r[1], c->r[2], c->r[3],
           c->flags.z, c->flags.c, c->flags.n, c->flags.v);
}

void cpu_clock(struct Cpu *c)
{
    struct AluResult res;

    if (c->halted) {
        return;
    }

    dump(c);
    c->cycles++;

    switch (c->state) {
    case ST_FETCH:
        /* 用 PC 当地址去指令存储器取 16 位数，放进 IR。
         * 然后 PC+1：默认下一条按顺序执行。跳转会在 EXECUTE 里改写 PC。 */
        c->ir = c->imem[c->pc];
        c->pc = (uint8_t)(c->pc + 1);
        c->state = ST_DECODE;
        break;

    case ST_DECODE:
        /* 位运算把 16 位切成字段。>> 右移对齐到最低位，& 掩码只留该字段。 */
        c->op  = (c->ir >> 12) & 0xF;  /* 高 4 位 */
        c->rd  = (c->ir >> 10) & 0x3;
        c->rs  = (c->ir >> 8) & 0x3;
        c->rt  = (c->ir >> 6) & 0x3;
        c->imm = (uint8_t)(c->ir & 0xFF);
        c->reg_we = 0; /* 默认不写寄存器；要写的指令自己在 EXECUTE 置 1 */
        if (c->op == OP_HALT) {
            c->state = ST_HALT;
            c->halted = 1;
            break;
        }
        c->state = ST_EXECUTE;
        break;

    case ST_EXECUTE:
        switch (c->op) {
        case OP_NOP:
            break;
        case OP_MOVI:
            c->wb_val = c->imm;
            c->reg_we = 1;
            break;
        case OP_MOV:
            c->wb_val = c->r[c->rs];
            c->reg_we = 1;
            break;
        case OP_ADD:
            res = alu_exec(ALU_ADD, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_SUB:
            res = alu_exec(ALU_SUB, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_AND:
            res = alu_exec(ALU_AND, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_OR:
            res = alu_exec(ALU_OR, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_ADDI:
            /* 目的寄存器也当源：rd = rd + imm */
            res = alu_exec(ALU_ADD, c->r[c->rd], c->imm);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_SUBI:
            res = alu_exec(ALU_SUB, c->r[c->rd], c->imm);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_CMP:
            /* 减法只为了改标志，结果扔掉（reg_we 保持 0） */
            res = alu_exec(ALU_SUB, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            break;
        case OP_LOAD:
            c->wb_val = c->dmem[c->r[c->rs]]; /* 地址在寄存器里，叫「寄存器间接寻址」 */
            c->reg_we = 1;
            break;
        case OP_STORE:
            c->dmem[c->r[c->rs]] = c->r[c->rt];
            break;
        case OP_JMP:
            c->pc = c->imm; /* 覆盖 FETCH 里已经 +1 过的 PC */
            break;
        case OP_JZ:
            if (c->flags.z) {
                c->pc = c->imm;
            }
            break;
        case OP_JNZ:
            /* 循环：SUBI 把计数器减到 0 时 Z=1，JNZ 不再跳，掉下去执行 HALT */
            if (!c->flags.z) {
                c->pc = c->imm;
            }
            break;
        default:
            break;
        }
        c->state = ST_WRITEBACK;
        break;

    case ST_WRITEBACK:
        if (c->reg_we) {
            c->r[c->rd] = c->wb_val;
        }
        c->state = ST_FETCH; /* 一条指令结束，回去取下一条 */
        break;

    case ST_HALT:
        c->halted = 1;
        break;
    }
}

int cpu_run(struct Cpu *c, int max_cycles)
{
    while (!c->halted && c->cycles < max_cycles) {
        cpu_clock(c);
    }
    return c->halted;
}

/*
 * R0 = 5+4+3+2+1 = 15
 *
 *   0  MOVI R0, 0
 *   1  MOVI R1, 5
 *   2  ADD  R0, R0, R1     ← 循环头，JNZ 跳回这里
 *   3  SUBI R1, 1          ← 减到 0 时 Z=1
 *   4  JNZ  2
 *   5  HALT
 */
void load_sum(struct Cpu *c)
{
    cpu_reset(c);
    c->imem[0] = enc_i(OP_MOVI, R0, 0);
    c->imem[1] = enc_i(OP_MOVI, R1, 5);
    c->imem[2] = enc_r(OP_ADD, R0, R0, R1);
    c->imem[3] = enc_i(OP_SUBI, R1, 1);
    c->imem[4] = enc_i(OP_JNZ, 0, 2);
    c->imem[5] = enc_i(OP_HALT, 0, 0);
}

/*
 * 斐波那契迭代 6 次：F0=0, F1=1 → F7=13，结果在 R1。
 *
 *   R3 = R0 + R1
 *   R0 = R1
 *   R1 = R3
 */
void load_fib(struct Cpu *c)
{
    cpu_reset(c);
    c->imem[0] = enc_i(OP_MOVI, R0, 0);
    c->imem[1] = enc_i(OP_MOVI, R1, 1);
    c->imem[2] = enc_i(OP_MOVI, R2, 6);
    c->imem[3] = enc_r(OP_ADD, R3, R0, R1);
    c->imem[4] = enc_r(OP_MOV, R0, R1, 0);
    c->imem[5] = enc_r(OP_MOV, R1, R3, 0);
    c->imem[6] = enc_i(OP_SUBI, R2, 1);
    c->imem[7] = enc_i(OP_JNZ, 0, 3);
    c->imem[8] = enc_i(OP_HALT, 0, 0);
}

/* 把 dmem[0x10..0x13] 拷到 dmem[0x20..0x23]。源数据先手写进内存。 */
void load_memcpy(struct Cpu *c)
{
    cpu_reset(c);
    c->dmem[0x10] = 0xAA;
    c->dmem[0x11] = 0xBB;
    c->dmem[0x12] = 0xCC;
    c->dmem[0x13] = 0xDD;
    c->imem[0] = enc_i(OP_MOVI, R0, 0x10);          /* 源地址 */
    c->imem[1] = enc_i(OP_MOVI, R1, 0x20);          /* 目的地址 */
    c->imem[2] = enc_i(OP_MOVI, R2, 4);             /* 还剩几个字节 */
    c->imem[3] = enc_r(OP_LOAD, R3, R0, 0);         /* R3 = mem[R0] */
    c->imem[4] = enc_r(OP_STORE, 0, R1, R3);        /* mem[R1] = R3 */
    c->imem[5] = enc_i(OP_ADDI, R0, 1);
    c->imem[6] = enc_i(OP_ADDI, R1, 1);
    c->imem[7] = enc_i(OP_SUBI, R2, 1);
    c->imem[8] = enc_i(OP_JNZ, 0, 3);
    c->imem[9] = enc_i(OP_HALT, 0, 0);
}
