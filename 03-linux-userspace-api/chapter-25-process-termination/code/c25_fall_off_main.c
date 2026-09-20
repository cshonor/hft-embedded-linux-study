/* c25_fall_off_main.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 §25.1 末尾提到一件容易忽略的事：从 main() **掉出末尾**（既没 return
   也没调 exit()）时，调用 main() 的那段运行时（crt0 里的 __libc_start_main）
   会接着调 exit()，但**用什么参数**取决于 C 标准和编译选项：

     · C89：行为**未定义**，程序可能带任意状态值终止。原书点名 gcc 默认就是
       这种情形 —— 退出状态取自「栈上或某个 CPU 寄存器里的某个随机值」。
     · C99：掉出 main 末尾**等价于 exit(0)**。原书说用 `gcc -std=c99` 就能拿到
       这个行为。

   本程序故意写成一个没有 return 的 main，然后**用同一个源文件、只改 -std**
   编两遍，把两次的退出码并列出来。

   为了让「那个随机值」可指认，main 的最后一句是 `marker()`（返回 42 的函数）
   —— 它的返回值没人接，很可能就留在返回寄存器里，被 crt0 当成 main 的返回值。
   ⚠️ 这只是**实现细节**，不是标准保证的东西；C89 下真正的语义就是「未定义」。

   编译（两个作业）:
     gcc -O0 -Wall -Wextra -std=c89 -o c25_fall_off_c89 c25_fall_off_main.c
     gcc -O0 -Wall -Wextra -std=c99 -o c25_fall_off_c99 c25_fall_off_main.c
   运行: ./c25_fall_off_c89 ; echo $?     （看退出码）
*/
#include <string.h>
#include <unistd.h>

static const char MSG[] =
    "main() 走完了最后一条语句：没有 return，也没有调 exit()\n"
    "本程序的退出码，就是 crt0 拿到的那个「main 的返回值」\n";

static int
marker(void)
{
    return 42;      /* 故意留一个可指认的数字 */
}

int
main(void)
{
    write(STDOUT_FILENO, MSG, strlen(MSG));
    marker();       /* 最后一句：返回值无人接收 */
}
