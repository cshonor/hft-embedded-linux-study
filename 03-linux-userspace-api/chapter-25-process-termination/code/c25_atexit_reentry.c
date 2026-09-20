/* c25_atexit_reentry.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 §25.3 在讲 atexit() 的调用顺序时，插了一句很容易被略过的话：

     "Essentially, any desired action can be performed inside an exit handler,
      including registering additional exit handlers, which are placed at the
      head of the list of exit handlers that remain to be called."

   「placed at the head of the list that remain to be called」—— 这不是
   「追加到队尾」，而是**插队到队首**。本程序把这个语义做成可观察量：
   main 里依次注册 A、B、C（⇒ 逆序跑：C、B、A），然后在 B 里面再注册一个 D。
   如果 D 是「追加到队尾」，顺序会是 C、B、A、D；
   如果 D 是「插到队首」，顺序会是 C、B、D、A。

   在 CE 上跑：stdout 不是终端 ⇒ 全缓冲 ⇒ 所有 puts/printf 都先攒在缓冲里，
   最后由 exit() 第 ② 步一次性写出，所以**打印顺序 == handler 执行顺序**。

   编译: gcc -O0 -Wall -Wextra -o c25_atexit_reentry c25_atexit_reentry.c
   运行: ./c25_atexit_reentry
*/
#include <stdio.h>
#include <stdlib.h>

static void
handlerD(void)
{
    printf("  handler D   ← 这个是在 handler B 里才注册的\n");
}

static void
handlerC(void)
{
    printf("  handler C\n");
}

static void
handlerB(void)
{
    printf("  handler B   ← 就在这一行里又注册了 D\n");
    if (atexit(handlerD) != 0) {
        printf("  !! 在 handler 里注册 D 失败\n");
    }
}

static void
handlerA(void)
{
    printf("  handler A\n");
}

int
main(void)
{
    printf("=== 注册顺序：A、B、C ⇒ 逆序执行：C、B、A ===\n");
    printf("=== 但 B 会在自己运行期间再注册一个 D ===\n\n");

    if (atexit(handlerA) != 0 || atexit(handlerB) != 0 || atexit(handlerC) != 0) {
        printf("atexit 注册失败\n");
        return EXIT_FAILURE;
    }

    printf("--- main 调 exit(0)，下面三行/四行的次序就是答案 ---\n");
    exit(0);    /* 不返回 */
}
