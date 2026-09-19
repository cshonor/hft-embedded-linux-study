/* T12: assembly file (.S) calling a C function — x86-64 SysV ABI
 *
 * .S extension (uppercase) => gcc runs the preprocessor, so #include etc. work.
 * .s extension (lowercase) => no preprocessing.
 *
 * Pattern: .global _start or your entry, set up args in rdi/rsi, call C func.
 * On Linux we link against libc and use main as the C entry; here we show a
 * small asm helper that calls a C function, demonstrating the ABI handshake.
 */

        .text
        .globl asm_call_c
        .type  asm_call_c, @function
asm_call_c:
        /* C signature: int c_add(int a, int b)
         * ABI: a -> edi, b -> esi, return -> eax
         * Our asm_call_c(a,b) just forwards — but to PROVE we control the ABI,
         * we shuffle the args. */
        mov    %edi, %eax       /* a -> eax (will become arg1) */
        mov    %esi, %ecx       /* b -> ecx */

        /* now call c_add with a,b swapped */
        mov    %ecx, %edi       /* first arg = original b */
        mov    %eax, %esi       /* second arg = original a */
        call   c_add            /* returns in eax */
        ret
        .size  asm_call_c, .-asm_call_c

/* Section note: .S files support macros. The Linux kernel uses .S heavily
 * for entry code (arch/x86/entry/entry_64.S), head code, and alternatives. */
