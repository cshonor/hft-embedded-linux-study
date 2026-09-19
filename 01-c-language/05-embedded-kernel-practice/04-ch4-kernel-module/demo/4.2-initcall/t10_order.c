/* t10_order.c —— main + do_initcalls，验证链接顺序决定执行顺序
 * 不定义 a/b/c_driver，它们在各自源文件里。
 * 链接顺序: gcc t10_a.o t10_b.o t10_c.o t10_order.o  → a b c
 *           gcc t10_c.o t10_b.o t10_a.o t10_order.o  → c b a
 */
#include <stdio.h>
typedef int (*initcall_t)(void);
extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

int main(void)
{
    printf("T10: initcall link order\n");
    initcall_t *fn;
    int i = 0;
    for (fn = __initcall_start; fn < __initcall_end; fn++) {
        printf("  [%d] %p -> ", i++, (void*)*fn);
        (*fn)();
    }
    printf("  link order determines execution order within a level\n");
    return 0;
}
