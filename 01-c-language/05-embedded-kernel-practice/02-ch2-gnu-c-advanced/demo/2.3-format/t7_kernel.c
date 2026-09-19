/* T7d: kernel extended conversions (%pK %pI4 %pS) vs the printf/gnu_printf archetypes */
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void kprintf_gnu(const char *fmt, ...) __attribute__((format(gnu_printf, 1, 2)));

int main(void)
{
    void *p = 0;
    kprintf("%pK\n", p);      /* A: kernel hashed pointer   */
    kprintf_gnu("%pK\n", p);  /* B: same under gnu_printf   */
    kprintf("%pI4\n", p);     /* C: kernel IPv4             */
    kprintf("%pS\n", p);      /* D: kernel symbol           */
    kprintf("%p\n", p);       /* E: plain %p is fine        */
    return 0;
}
