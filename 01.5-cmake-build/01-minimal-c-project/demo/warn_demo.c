/* warn_demo.c —— 专门用来"养"编译器警告的演示文件。
 * 三个小毛病都是新手最常见的，-Wall / -Wextra / -Werror 各有反应。
 * 编译它对比：
 *   cc -c warn_demo.c -o /dev/null              ← 默认几乎不吭声
 *   cc -Wall -c ...                              ← 开常用警告
 *   cc -Wall -Wextra -c ...                      ← 连可疑细节一起报
 *   cc -Wall -Wextra -Werror -c ...              ← 警告升级成错误，编不过
 */
#include <stdio.h>

int unused_helper(int x)            /* 毛病①：定义了但从没被调用 */
{
    return x * 2;
}

int main(void)
{
    int price;                      /* 毛病②：没初始化就用 */
    int qty = 3;
    char c = 'A';
    double total;

    if (qty = 5)                    /* 毛病③：把 = 当 == 用（赋值！不是比较） */
        printf("qty is 5\n");

    unsigned balance = 1;
    if (balance > qty)              /* 毛病④：无符号和有符号比较（qty=-1 时判断会反直觉） */
        printf("enough\n");

    total = price * qty;            /* 用了未初始化的 price */
    printf("%c total=%f\n", c, total);
    (void)unused_helper;
    return 0;
}
