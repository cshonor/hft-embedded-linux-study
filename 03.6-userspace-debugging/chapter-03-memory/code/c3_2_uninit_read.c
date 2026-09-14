/*
 * c3_2_uninit_read.c —— 未初始化读：ASan 看不见，valgrind / MSan 才看得见
 *
 * 用途：3.2 里有一节「ASan 不查值合法性」。光写结论读者记不住，
 *       这份程序就是那个结论的实体：同一份代码
 *         · 加 -fsanitize=address  → 干干净净退出 0
 *         · 加 -fsanitize=memory   → MSan 报 use-of-uninitialized-value
 *
 * 编译（ASan，期望「什么都不报」）：
 *   gcc -g -O0 -fsanitize=address -o c3_2_asan c3_2_uninit_read.c
 * 编译（MSan，只有 clang 有，且需要整条依赖链都用 MSan 重编）：
 *   clang -g -O1 -fsanitize=memory -fno-omit-frame-pointer -o c3_2_msan c3_2_uninit_read.c
 */
#include <stdio.h>
#include <stdlib.h>

struct order {
    long id;
    long qty;
    long price;
};

static volatile long g_sink;

int main(void)
{
    int stack_var;                       /* 未初始化：值 = 栈上的残留 */
    struct order *o = malloc(sizeof *o); /* 未初始化：值 = 堆上的残留 */
    if (!o)
        return 1;

    o->id = 1;
    /* 注意：qty / price 故意不赋值 */

    printf("stack_var = %d\n", stack_var);
    printf("o->qty = %ld, o->price = %ld\n", o->qty, o->price);

    g_sink = stack_var + o->qty;

    if (o->qty > 0)                      /* 用垃圾值做分支：行为不可预测 */
        printf("qty > 0 分支被走到（这完全取决于栈/堆里剩了什么）\n");
    else
        printf("qty > 0 分支没走到（同上）\n");

    free(o);
    return 0;
}
