/* t_getpwent.c —— 原书 Ch08 配套程序复刻版
 *
 * 演示用 getpwent() 顺序遍历系统密码文件。
 * 原书依赖 tlpi_hdr.h（取 EXIT_SUCCESS 等），此处只补 <stdlib.h>，其余逐行原样。
 *
 * 编译：cc -Wall -Wextra -o t_getpwent t_getpwent.c
 */
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    struct passwd *pwd;

    while ((pwd = getpwent()) != NULL)
        printf("%-8s %5ld\n", pwd->pw_name, (long) pwd->pw_uid);
    endpwent();
    exit(EXIT_SUCCESS);
}
