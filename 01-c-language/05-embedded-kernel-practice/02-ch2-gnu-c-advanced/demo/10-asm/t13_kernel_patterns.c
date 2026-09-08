/* T13: kernel-style inline asm patterns we will see in real source
 *
 * Reading aid: these are the idioms the Linux kernel uses. We verify each
 * compiles and (where possible) emits the expected instruction.
 */
#include <stdio.h>
#include <stdint.h>

/* (1) compiler barrier — no instruction, forces reload */
#define barrier() asm volatile("" ::: "memory")

/* (2) full CPU barrier on x86 = mfence; on ARM = dmb ish (not tested here) */
static inline void mb(void)  { asm volatile("mfence" ::: "memory"); }
static inline void rmb(void) { asm volatile("lfence" ::: "memory"); }
static inline void wmb(void) { asm volatile("sfence" ::: "memory"); }

/* (3) rdtsc — read timestamp counter. Two outputs in edx:eax. */
static inline uint64_t rdtsc(void)
{
    unsigned lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* (4) rdtscp — serializing variant, also returns CPU id in ecx */
static inline uint64_t rdtscp(uint32_t *cpu)
{
    unsigned lo, hi, aux;
    asm volatile ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    *cpu = aux;
    return ((uint64_t)hi << 32) | lo;
}

/* (5) xchg — atomic swap, full barrier on x86 (implicit lock prefix) */
static inline long xchg(volatile long *p, long v)
{
    /* the "=r" output and "1" tie make gcc reuse the same reg for v and out */
    long out;
    asm volatile (
        "xchg %0, %1"
        : "=r"(out), "+m"(*p)
        : "0"(v)
        : "memory"
    );
    return out;
}

/* (6) cmpxchg — CAS. The classic lock primitive. */
static inline int cmpxchg(volatile int *p, int old, int new)
{
    int cur;
    /* result in eax tells us if the swap happened */
    asm volatile (
        "lock cmpxchg %2, %1\n\t"
        "mov %3, %0"             /* capture: eax (old seen) into cur */
        : "=r"(cur)
        : "m"(*p), "r"(new), "a"(old)
        : "cc", "memory"
    );
    /* cur == old means success */
    return cur == old;
}

/* (7) get_cpu_id via cpuid (leaf 0) */
static inline void cpuid0(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(0)
    );
}

int main(void)
{
    barrier();
    mb(); rmb(); wmb();

    uint64_t t1 = rdtsc();
    uint32_t cpu;
    uint64_t t2 = rdtscp(&cpu);
    printf("rdtsc  = %lu\nrdtscp = %lu (cpu=%u)\n",
           (unsigned long)t1, (unsigned long)t2, cpu);

    long val = 1;
    long old = xchg(&val, 100);
    printf("xchg: old=%ld new=%ld\n", old, val);

    int slot = 0;
    int ok = cmpxchg(&slot, 0, 1);   /* should succeed */
    printf("cmpxchg(0->1) ok=%d slot=%d\n", ok, slot);
    int ok2 = cmpxchg(&slot, 0, 2);   /* should fail */
    printf("cmpxchg(0->2) ok=%d slot=%d\n", ok2, slot);

    uint32_t a, b, c, d;
    cpuid0(&a, &b, &c, &d);
    printf("cpuid0: max_leaf=%u vendor=%c%c%c%c%c%c%c%c%c%c%c%c\n",
           a,
           b & 0xff, (b>>8)&0xff, (b>>16)&0xff, (b>>24)&0xff,
           d & 0xff, (d>>8)&0xff, (d>>16)&0xff, (d>>24)&0xff,
           c & 0xff, (c>>8)&0xff, (c>>16)&0xff, (c>>24)&0xff);
    return 0;
}
