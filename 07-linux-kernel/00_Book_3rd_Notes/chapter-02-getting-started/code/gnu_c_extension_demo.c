/* GNU C 扩展 vs 纯 ISO C（演示 ?: 与语句表达式）
 *
 * 应通过:
 *   gcc -std=gnu11 -Wall -o gnu_c_extension_demo gnu_c_extension_demo.c
 *
 * 严格标准模式会抱怨（ pedantic ）:
 *   gcc -std=c11 -pedantic -Wall -c gnu_c_extension_demo.c
 *
 * 要点: 内核用 -std=gnu11；这些写法不是「链了 glibc」，而是编译器方言。
 */
#include <stdio.h>

#define pick(a, b) ({          \
    typeof(a) _a = (a);        \
    typeof(b) _b = (b);        \
    _a ? _a : _b;              \
})

int main(void)
{
    int val = 0;
    int x = val ?: -1;           /* GNU: 省略中间项 */
    int y = pick(0, 42);         /* GNU: typeof + 语句表达式 */

    printf("x=%d y=%d\n", x, y);
    return 0;
}
