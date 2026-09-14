/* t_ugid.c —— 验收原书 Listing 8-1 的四个助手函数
 *
 * 这四个函数（userNameFromId / userIdFromName / groupNameFromId /
 * groupIdFromName）是 TLPI 后面十几章都在用的公共件，所以单独验一遍：
 *   - 数字字符串走「快捷路径」，不查数据库
 *   - 查不到时返回 NULL / (uid_t)-1
 *   - 主机上没有的目标（比如没有 /etc/group 的容器）必须能安全返回
 *
 * 编译（三个文件一起）：
 *   cc -Wall -Wextra -o t_ugid t_ugid.c ugid_functions.c
 */
#include <stdio.h>
#include <stdlib.h>

#include "ugid_functions.h"

int main(void)
{
    const char *names[] = { "ce", "1000", "0", "nosuchuser", "" };
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        uid_t u = userIdFromName(names[i]);
        gid_t g = groupIdFromName(names[i]);
        char *un = userNameFromId(u);
        char *gn = groupNameFromId(g);
        printf("userIdFromName(\"%s\") = %-6ld  userNameFromId -> %s\n",
               names[i], (long) u, un ? un : "(NULL)");
        printf("          groupIdFromName = %-6ld  groupNameFromId -> %s\n",
               (long) g, gn ? gn : "(NULL)");
    }

    /* 边界：NULL 指针与「UID 存在但没有对应记录」 */
    printf("\nuserIdFromName(NULL) = %ld\n", (long) userIdFromName(NULL));
    printf("groupNameFromId(10240) = %s\n",
           groupNameFromId(10240) ? groupNameFromId(10240) : "(NULL)");
    return 0;
}
