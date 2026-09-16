/* c17_1_acl_algorithm.c — Ch17 §17.2：ACL 权限判定算法的纯用户态实现
 *
 * ⚠️ POSIX draft ACL 的内核实现（libacl / system.* xattr）是 Linux 专有；
 *    但 §17.2 的**判定算法本身**是纯逻辑，可以在用户态完整复现并实测。
 *
 * 算法（TLPI §17.2，与 POSIX 1003.1e draft 17 一致）：
 *   ① euid == 文件属主        → ACL_USER_OBJ 三元组，结束（mask 不参与）
 *   ② euid 命中某条 ACL_USER  → 该条 AND mask（若有 mask）；结束
 *   ③ egid == 文件属组 或 补充组 ∩ {ACL_GROUP_OBJ, 命中的 ACL_GROUP}
 *      → 命中可能多条，取"交完 mask 后任一允许"（OR）；结束
 *   ④ 都没命中 → ACL_OTHER（mask 不参与）
 *
 * 关键点（书上的强调 + 本 demo 实测）：
 *   - 属主**只看** ACL_USER_OBJ：即使 named user 给了更大权限，属主也用不上；
 *   - mask 是 named user / named group / group obj 的**上限**，
 *     对 owner 与 other **无效**；
 *   - 组类多条命中是 **OR**（一条交完 mask 后允许即允许）。
 *
 * 编译： cc -Wall -Wextra -o c17_1_acl_algorithm c17_1_acl_algorithm.c
 * 取材： TLPI §17.2（判定算法）；POSIX 1003.1e draft 17 §23.4.24
 */
#include <stdio.h>
#include <stdlib.h>

enum { ACL_USER_OBJ, ACL_USER, ACL_GROUP_OBJ, ACL_GROUP, ACL_MASK, ACL_OTHER };
enum { P_READ = 4, P_WRITE = 2, P_EXEC = 1 };

typedef struct { int type, id, perms; } Ace;
typedef struct { Ace *entries; int n; int has_mask; } Acl;

static const Ace *find(const Acl *a, int type, int id)
{
    for (int i = 0; i < a->n; i++)
        if (a->entries[i].type == type && a->entries[i].id == id)
            return &a->entries[i];
    return NULL;
}

/* uid 的补充组由 supp[] 给出；owner_id / owning_gid 是文件 inode 的属主/属组 */
static int acl_check(const Acl *acl, int uid, const int *supp, int nsupp,
                     int owner_id, int owning_gid, int want, const char **why)
{
    const Ace *mask = acl->has_mask ? find(acl, ACL_MASK, 0) : NULL;
    int cap = mask ? mask->perms : ~0;

    /* ① 属主 */
    if (uid == owner_id) {
        const Ace *e = find(acl, ACL_USER_OBJ, 0);
        *why = "① euid==属主 → ACL_USER_OBJ（mask 不参与）";
        return (e->perms & want) == want ? 0 : -1;
    }
    /* ② named user */
    {
        const Ace *e = find(acl, ACL_USER, uid);
        if (e) {
            *why = acl->has_mask ? "② 命中 named user → 该条 AND mask"
                                 : "② 命中 named user（无 mask）";
            return ((e->perms & cap) & want) == want ? 0 : -1;
        }
    }
    /* ③ 组类：group obj（egid 或补充组 == owning_gid）+ named group（OR） */
    {
        int in_owning_group = 0;
        for (int i = 0; i < nsupp; i++)
            if (supp[i] == owning_gid) in_owning_group = 1;
        int allow = 0, matched = 0;
        for (int i = 0; i < acl->n; i++) {
            const Ace *e = &acl->entries[i];
            int hit = 0;
            if (e->type == ACL_GROUP_OBJ)
                hit = in_owning_group;
            else if (e->type == ACL_GROUP) {
                for (int g = 0; g < nsupp; g++)
                    if (supp[g] == e->id) hit = 1;
            }
            if (hit) {
                matched = 1;
                if (((e->perms & cap) & want) == want) allow = 1;
            }
        }
        if (matched) {
            *why = acl->has_mask ? "③④ 组类命中 → 该条 AND mask（多条 OR）"
                                 : "③④ 组类命中（无 mask，多条 OR）";
            return allow ? 0 : -1;
        }
    }
    /* ④ other */
    {
        const Ace *e = find(acl, ACL_OTHER, 0);
        *why = "④ 未命中 → ACL_OTHER（mask 不参与）";
        return (e->perms & want) == want ? 0 : -1;
    }
}

int main(void)
{
    /* 典型 ACL（对应 setfacl 文本形式）：
     *   user::rw-  user:1001:r--  group::r--  group:1002:-wx  mask::rw-  other::r--
     * 文件属主=1000，属组=1003
     */
    Ace entries[] = {
        { ACL_USER_OBJ,  0,    P_READ | P_WRITE },
        { ACL_USER,      1001, P_READ },
        { ACL_GROUP_OBJ, 0,    P_READ },
        { ACL_GROUP,     1002, P_WRITE | P_EXEC },
        { ACL_MASK,      0,    P_READ | P_WRITE },
        { ACL_OTHER,     0,    P_READ },
    };
    Acl acl = { entries, 6, 1 };
    const int OWNER = 1000, OWNING_GID = 1003;

    int g_1002[] = { 1002 };
    int g_1003[] = { 1003 };
    int g_none[] = { 0 };
    struct { const char *who; int uid, *g, ng, want; } cases[] = {
        { "uid=1000 属主         求 r", 1000, g_none, 0, P_READ },
        { "uid=1000 属主         求 w", 1000, g_none, 0, P_WRITE },
        { "uid=1001 named user   求 r", 1001, g_none, 0, P_READ },
        { "uid=1001 named user   求 w", 1001, g_none, 0, P_WRITE },
        { "uid=1002 ∈组1002      求 w", 1002, g_1002, 1, P_WRITE },
        { "uid=1002 ∈组1002      求 x", 1002, g_1002, 1, P_EXEC },
        { "uid=2000 ∈组1003      求 r", 2000, g_1003, 1, P_READ },
        { "uid=2000 路人         求 r", 2000, g_none, 0, P_READ },
        { "uid=2000 路人         求 w", 2000, g_none, 0, P_WRITE },
    };

    printf("== ACL: user::rw- user:1001:r-- group::r-- group:1002:-wx mask::rw- other::r-- ==\n");
    printf("== 文件属主=1000，属组=1003 ==\n\n");
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        const char *why = "";
        int r = acl_check(&acl, cases[i].uid, cases[i].g, cases[i].ng,
                          OWNER, OWNING_GID, cases[i].want, &why);
        printf("  %-24s → %-4s  [%s]\n", cases[i].who, r == 0 ? "允许" : "拒绝", why);
    }

    printf("\n== mask 清零：named/组全灭，属主与 other 不受影响 ==\n");
    for (int i = 0; i < acl.n; i++)
        if (entries[i].type == ACL_MASK) entries[i].perms = 0;
    const char *why = "";
    printf("  属主 1000 求 r  → %s（mask 对 owner 无效，仍走 rw-）\n",
           acl_check(&acl, 1000, g_none, 0, OWNER, OWNING_GID, P_READ, &why) == 0 ? "允许" : "拒绝");
    printf("  named 1001 求 r → %s（mask 生效，r-- 交 0 = 0）\n",
           acl_check(&acl, 1001, g_none, 0, OWNER, OWNING_GID, P_READ, &why) == 0 ? "允许" : "拒绝");
    printf("  路人 2000 求 r  → %s（mask 对 other 无效，仍走 r--）\n",
           acl_check(&acl, 2000, g_none, 0, OWNER, OWNING_GID, P_READ, &why) == 0 ? "允许" : "拒绝");
    return EXIT_SUCCESS;
}
