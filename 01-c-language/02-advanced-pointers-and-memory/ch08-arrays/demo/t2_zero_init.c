/* t2_zero_init.c —— `= {0}` 到底做了什么（8.1.8 / 8.1.9 实测底稿）
 *
 * 回答三个问题：
 *   1. `int a[10] = {0}` 是不是整个数组都变 0？
 *   2. `{0}` 里的 0 是什么？是不是「一个整体的 0」？
 *   3. 编译成什么？（memset / rep stos / 向量指令）
 *
 * WSL: gcc -std=c11 -O0 -Wall -o t2 t2_zero_init.c && ./t2
 */
#include <stdio.h>
#include <string.h>

/* 全局一览：看 .data / .bss 分流 */
int  g_zero[10]  = {0};      /* .data（显式初始化，虽然全是 0） */
int  g_one[10]   = {1};      /* .data：g_one[0]=1，其余 0 */
int  g_none[10];             /* .bss：全 0，由内核清零 */
static int g_static[10];     /* .bss */

static void dump(const char *tag, const int *arr, int n)
{
    printf("  %-28s = {", tag);
    for (int i = 0; i < n; i++)
        printf("%d%s", arr[i], i == n - 1 ? "" : ", ");
    printf("}\n");
}

int main(void)
{
    /* ---- 1. 栈上的四种写法 ---- */
    int a[10] = {0};          /* 全 0 */
    int b[10] = {1};          /* 只有 b[0]=1 */
    int c[10] = {7, 8, 9};    /* c[0..2] 有值 */
    int d[10];                /* 脏值（栈） */

    printf("======== 1. 栈数组：{0} / {1} / {7,8,9} / 不写 ========\n");
    dump("a[10] = {0}", a, 10);
    dump("b[10] = {1}", b, 10);
    dump("c[10] = {7,8,9}", c, 10);
    printf("  d[10];                       = 未初始化（栈脏值，不打印）\n");
    printf("  注：b 的其余 9 个是 0 —— 这就证明 {} 里是「按位置对应的值」，\n");
    printf("      不是「一个整体的 0」。若是一个总的 0，b 就该全是 1 了。\n\n");

    /* ---- 2. 全局/静态：.bss 免费清零 ---- */
    printf("======== 2. 全局数组 ========\n");
    dump("g_zero[10] = {0}", g_zero, 10);
    dump("g_one[10]  = {1}", g_one, 10);
    dump("g_none[10]（无初始化）", g_none, 10);
    dump("g_static[10]（无初始化）", g_static, 10);
    printf("  注：g_none / g_static 落在 .bss，加载时内核统一清零 ——\n");
    printf("      零条指令、零文件体积。见 readelf -S / size 输出。\n\n");

    /* ---- 3. 指定初始化（C99）也是「其余补 0」 ---- */
    int e[10] = {[5] = 99, [2] = 22};
    printf("======== 3. C99 指定初始化 ========\n");
    dump("e[10] = {[5]=99, [2]=22}", e, 10);
    printf("\n");

    /* ---- 4. 聚合成员：结构体数组也吃同一条规则 ---- */
    struct row { int id; char tag; };
    struct row rows[4] = {0};          /* 全 0 */
    struct row rows2[4] = {{1, 'x'}};  /* 只有第 0 个有值 */
    printf("======== 4. 结构体数组 ========\n");
    printf("  rows[4]  = {0}        -> ");
    for (int i = 0; i < 4; i++) printf("{%d,%d} ", rows[i].id, rows[i].tag);
    printf("\n  rows2[4] = {{1,'x'}}  -> ");
    for (int i = 0; i < 4; i++) printf("{%d,%d} ", rows2[i].id, rows2[i].tag);
    printf("\n\n");

    /* ---- 5. 清零 vs 显式 memset：语义等价，但含义不同 ---- */
    char buf1[64] = {0};
    char buf2[64]; memset(buf2, 0, sizeof buf2);
    printf("======== 5. {0} 与 memset 等价性 ========\n");
    printf("  memcmp(buf1, buf2, 64) = %d （0 表示逐字节相同）\n\n",
           memcmp(buf1, buf2, 64));

    /* ---- 6. 清零后立刻全量覆盖：编译器能否省掉？ ---- */
    char fill[128] = {0};                 /* 这里要清零吗？ */
    memset(fill, 'A', sizeof fill);       /* 立刻被完全覆盖 */
    printf("======== 6. 死存储消除（DSE）========\n");
    printf("  fill[0]=%c fill[127]=%c\n", fill[0], fill[127]);
    printf("  -O0: 会真的先清零；-O2: memset 被 DSE 消除（见 run.sh 汇编）\n");
    return 0;
}
