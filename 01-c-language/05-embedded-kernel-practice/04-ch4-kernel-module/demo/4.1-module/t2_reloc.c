/*
 * T2: 重定位公式 —— 内核 apply_relocate_add 的用户态缩影
 *
 * 内核 x86 的 write_relocate_add 核心就两行：
 *     val = sym->st_value + rel[i].r_addend;     // S + A
 *     if (PC相对)  val -= (u64)loc;              // S + A - P
 *     write(loc, &val, size);
 *
 * 本 demo 手搓一段“模块 .text”（一个 CALL e8 disp32），再用一条
 * R_X86_64_PC32 重定位把它指到一个目标函数，patch 后调用，验证公式。
 *
 * 32 位 PC 相对 CALL 的 disp32 = 目标地址 - (call 指令地址 + 5)
 *   即 S - (P + 5)。ELF 里这写成 R_X86_64_PC32 + addend=-4
 *   因为 r_offset 指向 disp 字段（call 的第 2 字节），P = r_offset 地址，
 *   disp = S + A - P = S + (-4) - P = S - (P + 4) = S - (call地址+5)。✓
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

/* 目标函数：模拟“内核导出的 printk”。无参，打固定串，
 * 让 demo 聚焦在 CALL 的重定位本身，不被 ABI 传参细节干扰。 */
__attribute__((noinline))
static void target_printk(void)
{
    printf("[resolved] module called printk via relocated call\n");
}

/* 模拟 ELF 重定位条目 */
struct fake_rela {
    uint64_t r_offset;   /* 要 patch 的位置在 .text 内的偏移 */
    uint32_t type;       /* R_X86_64_PC32 = 2 */
    uint32_t symidx;     /* 符号索引 */
    int64_t  addend;     /* A */
};

/* R_X86_64_* 类型号（出自 elf.h） */
#define R_X86_64_NONE   0
#define R_X86_64_64     1
#define R_X86_64_PC32   2
#define R_X86_64_PLT32  4
#define R_X86_64_32S    11

/* 模拟“模块的 .text”：一段手写机器码
 *   push 0          ; 6a 00        （占位，把字符串指针传进去）
 *   call <disp32>   ; e8 00 00 00 00   <- r_offset 指向这里的 disp32
 *   add rsp, 8      ; 48 83 c4 08
 *   ret             ; c3
 * 字符串 "from module" 由调用方放进栈（这里简化：先 push 一个立即数 0，
 * 真正 loader 会把它改成字符串地址 —— 但本 demo 只演示 CALL 的重定位）。
 *
 * 为了简化，模块函数不接收参数，只调用 target_printk("from module")。
 * 我们让 target_printk 自己用固定串。
 */
static unsigned char module_text[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,   /* call disp32  (offset 0, disp at offset 1) */
    0xc3                              /* ret */
};

int main(void)
{
    printf("=== T2: 重定位公式 S+A / S+A-P ===\n");

    /* 分配可执行内存，放“模块 .text”。
     * MAP_32BIT：把页放在低 2GB —— 模仿内核 -mcmodel=kernel 的约束：
     * 内核模块必须落在内核代码 ±2GB 内，PC32 重定位才不溢出。 */
    long pagesz = sysconf(_SC_PAGESIZE);
    void *text = mmap(NULL, pagesz, PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    if (text == MAP_FAILED) { perror("mmap"); return 1; }
    memcpy(text, module_text, sizeof(module_text));

    /* 构造一条 R_X86_64_PC32 重定位，指向 target_printk */
    struct fake_rela rela = {
        .r_offset = 1,                 /* disp32 在 .text 偏移 1 */
        .type     = R_X86_64_PC32,
        .symidx   = 0,
        .addend   = -4,               /* CALL 的经典 addend */
    };
    /* “符号表”：target_printk 的地址 */
    uint64_t sym_value = (uint64_t)(uintptr_t)&target_printk;

    /* patch 前的 disp32 */
    uint8_t *loc = (uint8_t *)text + rela.r_offset;
    int32_t before;
    memcpy(&before, loc, 4);
    printf("patch 前  disp32 = 0x%08x  (模块 call 未决议)\n", before);

    /* === 内核 write_relocate_add 的核心 === */
    int64_t val = (int64_t)sym_value + rela.addend;   /* val = S + A */
    if (rela.type == R_X86_64_PC32 || rela.type == R_X86_64_PLT32)
        val -= (int64_t)(uintptr_t)loc;                /* val = S + A - P */
    printf("公式: val = S + A - P = 0x%lx + (%ld) - 0x%lx = 0x%lx\n",
           (long)sym_value, (long)rela.addend, (long)(uintptr_t)loc, (long)val);
    /* 溢出检查（R_X86_64_PC32 是 32 位有符号） */
    if ((int64_t)(int32_t)val != val) {
        fprintf(stderr, "relocation overflow!\n");
        return 1;
    }
    int32_t disp = (int32_t)val;
    memcpy(loc, &disp, 4);                             /* write(loc, &val, 4) */

    /* patch 后的 disp32 */
    int32_t after;
    memcpy(&after, loc, 4);
    printf("patch 后  disp32 = 0x%08x\n", after);

    /* 验证：disp + (call地址+5) == target_printk ? */
    uint64_t call_addr = (uint64_t)(uintptr_t)text;   /* call 在偏移 0 */
    uint64_t resolved = call_addr + 5 + (int64_t)after;
    printf("运行期校验: call_addr+5+disp = 0x%lx  == target_printk 0x%lx ? %s\n",
           (long)resolved, (long)sym_value,
           resolved == sym_value ? "YES" : "NO");

    /* 改成可执行，调用它 —— 这就是 insmod 后 do_one_initcall 的缩影 */
    if (mprotect(text, pagesz, PROT_READ|PROT_EXEC) != 0) {
        perror("mprotect"); return 1;
    }
    printf("\n--- 调用 patch 后的模块代码 ---\n");
    typedef void (*fn_t)(void);
    fn_t module_entry = (fn_t)text;
    module_entry();   /* 应打印 [resolved] ... */

    munmap(text, pagesz);
    return 0;
}
