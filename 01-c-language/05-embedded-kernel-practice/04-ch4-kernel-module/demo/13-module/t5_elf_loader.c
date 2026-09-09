/*
 * T5: 最小真实 ELF 重定位加载器 —— “内核就是手写动态链接器”
 *
 * 流程对应 kernel/module/main.c 的 load_module：
 *   读 ELF → 找段 → 分配可执行内存(move_module) → resolve_symbol
 *   → apply_relocate_add(apply_relocations) → flush icache → 调入口(do_init_module)
 *
 * 本加载器处理 x86-64 可重定位 .o：
 *   - 解析 Elf64_Ehdr / Elf64_Shdr / Elf64_Sym / Elf64_Rela
 *   - 把 .text 复制到可执行页
 *   - 遍历 .rela.text，对每个重定位：
 *       符号已定义 → S = text_base + st_value
 *       符号未决(SHN_UNDEF) → resolve_symbol 查“内核符号表”拿地址
 *       val = S + A；PC32/PLT32 再 val -= P；写回 loc
 *   - 找 module_entry 符号，调 text_base + st_value
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

/* ===== “内核”提供的符号（模块要 resolve 的目标） ===== */
void kernel_print(void)
{
    printf("[kernel_print] 模块通过 ELF 重定位成功调到了我\n");
}

/* 极简“内核符号表”：名字 → 地址。对应 __ksymtab 的 bsearch 退化版 */
static void *resolve_symbol(const char *name)
{
    if (strcmp(name, "kernel_print") == 0) return (void *)kernel_print;
    return NULL;
}

static const char *reloc_name(unsigned type)
{
    switch (type) {
    case R_X86_64_NONE:   return "R_X86_64_NONE";
    case R_X86_64_64:     return "R_X86_64_64";
    case R_X86_64_PC32:   return "R_X86_64_PC32";
    case R_X86_64_PLT32:  return "R_X86_64_PLT32";
    case R_X86_64_32:     return "R_X86_64_32";
    case R_X86_64_32S:    return "R_X86_64_32S";
    default:              return "?";
    }
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "t5_module.o";
    printf("=== T5: ELF 重定位加载器，加载 %s ===\n\n", path);

    /* 读整个 .o 进内存（对应 copy_chunked_from_user） */
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    off_t flen = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    uint8_t *obj = malloc(flen);
    if (read(fd, obj, flen) != flen) { perror("read"); return 1; }
    close(fd);

    Elf64_Ehdr *eh = (Elf64_Ehdr *)obj;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "not an ELF\n"); return 1;
    }
    printf("ELF: e_type=%u (1=ET_REL 可重定位) e_shnum=%u e_shstrndx=%u\n\n",
           eh->e_type, eh->e_shnum, eh->e_shstrndx);

    Elf64_Shdr *sh = (Elf64_Shdr *)(obj + eh->e_shoff);
    const char *shstr = (const char *)(obj + sh[eh->e_shstrndx].sh_offset);

    /* 找 .text / .symtab / .strtab / .rela.text */
    int idx_text = -1, idx_symtab = -1, idx_strtab = -1, idx_reltext = -1;
    for (int i = 0; i < eh->e_shnum; i++) {
        const char *nm = shstr + sh[i].sh_name;
        if      (sh[i].sh_type == SHT_PROGBITS && strcmp(nm,".text")==0)   idx_text = i;
        else if (sh[i].sh_type == SHT_SYMTAB)                              idx_symtab = i;
        else if (sh[i].sh_type == SHT_RELA && strcmp(nm,".rela.text")==0)  idx_reltext = i;
    }
    if (idx_symtab >= 0) idx_strtab = sh[idx_symtab].sh_link; /* strtab 在 symtab 的 sh_link */
    printf("section: .text#%d .symtab#%d .strtab#%d .rela.text#%d\n",
           idx_text, idx_symtab, idx_strtab, idx_reltext);
    if (idx_text < 0 || idx_symtab < 0 || idx_strtab < 0) {
        fprintf(stderr, "缺必要段\n"); return 1;
    }

    Elf64_Shdr *text_sh = &sh[idx_text];
    Elf64_Sym  *syms = (Elf64_Sym *)(obj + sh[idx_symtab].sh_offset);
    int         nsym = sh[idx_symtab].sh_size / sizeof(Elf64_Sym);
    const char *strtab = (const char *)(obj + sh[idx_strtab].sh_offset);

    /* 打印符号表（区分 defined / undefined） */
    printf("\n--- 符号表（%d 个）---\n", nsym);
    for (int i = 1; i < nsym; i++) {
        const char *nm = strtab + syms[i].st_name;
        if (syms[i].st_name == 0) continue;
        printf("  [%2d] %-16s st_shndx=%d %s\n", i, nm, syms[i].st_shndx,
               syms[i].st_shndx == SHN_UNDEF ? "(UNDEF 未决)" : "");
    }

    /* 分配可执行页，复制 .text（对应 move_module）。
     * MAP_32BIT：让模块代码落在低 2GB，使 CALL 能用 PC32 重定位
     * 到本加载器里的 kernel_print（也在低地址）—— 模仿内核
     * 模块必须在内核 ±2GB 内的约束。 */
    long pagesz = sysconf(_SC_PAGESIZE);
    long allocsz = (text_sh->sh_size + pagesz - 1) & ~(pagesz - 1);
    uint8_t *text_base = mmap(NULL, allocsz ? allocsz : pagesz,
            PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    if (text_base == MAP_FAILED) { perror("mmap"); return 1; }
    memcpy(text_base, obj + text_sh->sh_offset, text_sh->sh_size);
    printf("\n.text 复制到可执行页 %p，size=%lu\n",
           (void*)text_base, (unsigned long)text_sh->sh_size);

    /* === apply_relocations === */
    if (idx_reltext >= 0) {
        Elf64_Shdr *rsh = &sh[idx_reltext];
        Elf64_Rela *rels = (Elf64_Rela *)(obj + rsh->sh_offset);
        int nrel = rsh->sh_size / sizeof(Elf64_Rela);
        printf("\n--- apply_relocate_add：%d 条重定位 ---\n", nrel);
        for (int i = 0; i < nrel; i++) {
            uint32_t sym = ELF64_R_SYM(rels[i].r_info);
            uint32_t typ = ELF64_R_TYPE(rels[i].r_info);
            int64_t  A   = rels[i].r_addend;
            uint8_t *loc = text_base + rels[i].r_offset;  /* P */
            const char *snm = strtab + syms[sym].st_name;

            /* 拿符号值 S */
            uint64_t S;
            if (syms[sym].st_shndx == SHN_UNDEF) {
                /* 未决符号 → resolve_symbol 查内核符号表 */
                S = (uint64_t)(uintptr_t)resolve_symbol(snm);
                if (!S) { fprintf(stderr,"resolve %s 失败\n", snm); return 1; }
                printf("  [%d] %s %s(UNDEF) -> 内核 resolve S=0x%lx",
                       i, reloc_name(typ), snm, (long)S);
            } else if (syms[sym].st_shndx == idx_text) {
                /* 已定义在 .text 段 → S = text_base + st_value */
                S = (uint64_t)(uintptr_t)(text_base + syms[sym].st_value);
                printf("  [%d] %s %s(defined) S=0x%lx",
                       i, reloc_name(typ), snm, (long)S);
            } else {
                S = syms[sym].st_value;
                printf("  [%d] %s %s S=0x%lx", i, reloc_name(typ), snm, (long)S);
            }

            /* 内核 write_relocate_add 的公式 */
            int64_t val = (int64_t)S + A;            /* val = S + A */
            int size = 8;
            switch (typ) {
            case R_X86_64_NONE:   continue;
            case R_X86_64_64:     size = 8; break;
            case R_X86_64_PC32:
            case R_X86_64_PLT32:  val -= (int64_t)(uintptr_t)loc; size = 4; break;
            case R_X86_64_32:     size = 4; break;
            case R_X86_64_32S:    size = 4; break;
            default: fprintf(stderr,"未知重定位 %u\n", typ); return 1;
            }
            if (size == 4) {
                int32_t v32 = (int32_t)val;
                memcpy(loc, &v32, 4);
            } else {
                int64_t v64 = val;
                memcpy(loc, &v64, 8);
            }
            printf(" -> 写 0x%lx 到 loc=%p (A=%ld)\n", (long)val, (void*)loc, (long)A);
        }
    }

    /* flush_module_icache：改可执行后刷新（x86 上 mprotect 即够） */
    if (mprotect(text_base, allocsz, PROT_READ|PROT_EXEC) != 0) {
        perror("mprotect exec"); return 1;
    }

    /* 找 module_entry 符号，调它（对应 do_init_module） */
    uint8_t *entry = NULL;
    for (int i = 1; i < nsym; i++) {
        const char *nm = strtab + syms[i].st_name;
        if (strcmp(nm, "module_entry") == 0) {
            entry = text_base + syms[i].st_value;
            break;
        }
    }
    if (!entry) { fprintf(stderr,"找不到 module_entry\n"); return 1; }

    printf("\n--- do_init_module：调 module_entry @ %p ---\n", (void*)entry);
    typedef int (*entry_t)(void);
    int ret = ((entry_t)entry)();
    printf("module_entry 返回 %d\n", ret);

    munmap(text_base, allocsz ? allocsz : pagesz);
    free(obj);
    return 0;
}
