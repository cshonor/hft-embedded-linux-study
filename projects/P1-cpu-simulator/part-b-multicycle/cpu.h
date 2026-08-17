#ifndef P1_CPU_H
#define P1_CPU_H

#include "alu.h"
#include "isa.h"

#include <stdint.h>

enum CpuState {
    ST_FETCH = 0,
    ST_DECODE,
    ST_EXECUTE,
    ST_WRITEBACK,
    ST_HALT
};

struct Cpu {
    uint8_t r[4];
    uint8_t pc;
    uint16_t ir;
    struct AluFlags flags;
    enum CpuState state;
    int halted;
    int cycles;
    int trace;

    uint16_t imem[256];
    uint8_t dmem[256];

    int op, rd, rs, rt;
    uint8_t imm;
    int reg_we;
    uint8_t wb_val;
};

void cpu_reset(struct Cpu *c);
void cpu_clock(struct Cpu *c);
int cpu_run(struct Cpu *c, int max_cycles);

void load_sum(struct Cpu *c);
void load_fib(struct Cpu *c);
void load_memcpy(struct Cpu *c);

#endif
