# Day 3 · Code layout

Examples are grouped by **book section**, with **English folder names** (avoids git push issues with Chinese paths).

| Folder | Section | What it demonstrates |
|--------|---------|----------------------|
| [sec-3.1-ipl-int13-disk-load](./sec-3.1-ipl-int13-disk-load/) | §3.1 | **512 B IPL** — `INT 0x13` reads bootpack → `JMP 0x8200` |
| [sec-3.2-vga-mode-0x13](./sec-3.2-vga-mode-0x13/) | §3.2 | **VGA 320×200×8** — lives inside `nasmhead.asm` (`INT 0x10`, AL=0x13) |
| [sec-3.4-bootpack-asm-and-c](./sec-3.4-bootpack-asm-and-c/) | §3.4 | **Four-file bootpack** — `nasmhead.asm` + `bootpack.c` + `asmfunc.asm` (+ IPL in §3.1) |
| [sec-3.4-minimal-16-to-32-call-c](./sec-3.4-minimal-16-to-32-call-c/) | §3.4.3 | **Minimal asm→C** — GDT/CR0 only, `kernel_main` instead of `HariMain` |

§3.3 is conceptual (16→32→64, Load vs Run) — see [notes](../notes/section-3.3-32-位模式前期准备与导入-C-语言.md), no separate code folder.

---

## End-to-end boot chain (Day 3)

```text
ipl.asm (§3.1)          nasmhead.asm (§3.4)       bootpack.c (§3.4)      asmfunc.asm (§3.4)
16-bit disk load   →    16→32 + VGA + call C  →   HariMain + io_hlt() →  HLT stub
```

Notes: [§3.4.2 four-file division](../notes/section-3.4.2-io_hlt与工程分层.md)
