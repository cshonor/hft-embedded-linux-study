/* T11: x86-64 System V ABI calling convention
 *
 * Linux x86-64 SysV ABI (used by gcc/clang on Linux, also macOS):
 *   - Integer/pointer args (first 6): RDI, RSI, RDX, RCX, R8, R9
 *   - Extra args: stack, pushed right-to-left
 *   - Return: RAX (and RDX for 128-bit)
 *   - Callee-saved: RBX, RBP, R12-R15  (must preserve)
 *   - Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11, R10, R11
 *   - Stack: 16-byte aligned before `call` (=> after push of return addr, 8 mod 16)
 *   - Red zone: 128 bytes below RSP usable without reservation (leaf functions, -O2)
 *
 * ARM AAPCS (for contrast, not run here — no cross toolchain):
 *   - Args (first 4): R0-R3 (AArch64: X0-X7)
 *   - Return: R0 (X0)
 *   - Callee-saved: R4-R11, R13 SP (X19-X29, SP)
 *   - Caller-saved: R0-R3, R12 IP, R14 LR (X0-X18, X30 LR)
 *   - Stack: 8-byte aligned (AArch64: 16-byte SP aligned)
 */
#include <stdio.h>
#include <stdint.h>

/* Verify: dump the register each arg lands in via inline asm.
 * 6 args -> rdi, rsi, rdx, rcx, r8, r9 */
__attribute__((noinline))
static void six_args(int a1, int a2, int a3, int a4, int a5, int a6)
{
    /* confirm ABI by reading what gcc actually did — we can't read rdi from
     * C, but we can inspect the disassembly */
    printf("six_args(%d,%d,%d,%d,%d,%d)\n", a1, a2, a3, a4, a5, a6);
}

/* Inline asm to inspect which register gcc put an arg in.
 * With "r" gcc picks; to FORCE a specific reg we name it. */
__attribute__((noinline))
static int force_rdi(int x)
{
    int o;
    asm volatile ("mov %%edi, %0" : "=r"(o) : "D"(x));   /* "D" = di/edi */
    return o;
}

/* 128-bit return via RDX:RAX */
__attribute__((noinline))
static __int128 big_return(void)
{
    return ((__int128)0x1122334455667788ULL << 64) | 0x99aabbccddeeff00ULL;
}

int main(void)
{
    six_args(1, 2, 3, 4, 5, 6);
    printf("force_rdi(42) = %d\n", force_rdi(42));
    __int128 r = big_return();
    printf("big_return low = 0x%llx high = 0x%llx\n",
           (unsigned long long)(uint64_t)r,
           (unsigned long long)(uint64_t)(r >> 64));
    return 0;
}
