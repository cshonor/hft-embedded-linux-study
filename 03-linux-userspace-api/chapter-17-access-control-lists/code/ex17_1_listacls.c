/* ex17_1_listacls.c — 习题 17-1：显示指定 user/group 对应 ACL 条目的权限
 *
 * 题面：程序接两个命令行参数：第一个是 u 或 g，第二个是用户/组（数字或
 * 名字均可）；显示该 user/group 对应 ACL 条目的权限；若该条目落入 mask
 * 作用范围，显示**与 mask 求交后的有效权限**；若无对应条目，说明其将
 * 落入 other/组类判定。
 *
 * ⚠️ 实测状态（诚实标注）：POSIX draft ACL 的完整实现（libacl，
 *    system.posix_acl_access xattr）是 **Linux 专有**；macOS 无对应
 *    常量/API，本文件**只在 Linux 编译运行**，Pi5(ext4) 是复测平台。
 *    判定算法的可运行版本见 c17_1_acl_algorithm.c（纯逻辑，全平台）。
 *
 * 编译（Linux/Pi5）： gcc -Wall -o ex17_1_listacls ex17_1_listacls.c -lacl
 * 取材： TLPI §17.1/§17.8 习题 17-1；结构沿用原书 acl_view.c 的成熟写法
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/acl.h>
#include <acl/libacl.h>         /* acl_get_perm_np() */
#include "tlpi_hdr.h"

static void perms_str(acl_permset_t ps, char *out)
{
    /* 逐位探测：r w x（draft 只定义这三个；ACL_MASK entry 另说） */
    out[0] = acl_get_perm_np(ps, ACL_READ)  == 1 ? 'r' : '-';
    out[1] = acl_get_perm_np(ps, ACL_WRITE) == 1 ? 'w' : '-';
    out[2] = acl_get_perm_np(ps, ACL_EXECUTE) == 1 ? 'x' : '-';
    out[3] = '\0';
}

static const char *tag_name(acl_tag_t t)
{
    switch (t) {
    case ACL_USER_OBJ:  return "user::";
    case ACL_USER:      return "user:";
    case ACL_GROUP_OBJ: return "group::";
    case ACL_GROUP:     return "group:";
    case ACL_MASK:      return "mask::";
    case ACL_OTHER:     return "other::";
    default:            return "?";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4 || argv[1][1] != '\0' ||
        (argv[1][0] != 'u' && argv[1][0] != 'g'))
        usageErr("%s u|g user-or-group file\n", argv[0]);

    const char *file = argv[3];
    int is_user = argv[1][0] == 'u';
    id_t want_id;

    if (is_user) {
        struct passwd *pw = getpwnam(argv[2]);
        want_id = pw ? pw->pw_uid : (uid_t) atoi(argv[2]);
    } else {
        struct group *gr = getgrnam(argv[2]);
        want_id = gr ? gr->gr_gid : (gid_t) atoi(argv[2]);
    }

    acl_t acl = acl_get_file(file, ACL_TYPE_ACCESS);
    if (acl == NULL)
        errExit("acl_get_file:%s（FS 需支持 POSIX draft ACL）", file);

    /* 第一遍：找 mask */
    acl_permset_t mask_ps = NULL;
    int have_mask = 0;
    acl_entry_t e;
    for (int st = acl_get_entry(acl, ACL_FIRST_ENTRY, &e);
         st == 1; st = acl_get_entry(acl, ACL_NEXT_ENTRY, &e)) {
        acl_tag_t t;
        acl_get_tag_type(e, &t);
        if (t == ACL_MASK) {
            acl_get_permset(e, &mask_ps);
            have_mask = 1;
        }
    }

    /* 第二遍：找目标条目 */
    int found = 0;
    for (int st = acl_get_entry(acl, ACL_FIRST_ENTRY, &e);
         st == 1; st = acl_get_entry(acl, ACL_NEXT_ENTRY, &e)) {
        acl_tag_t t;
        acl_get_tag_type(e, &t);
        id_t qual = 0;
        int hit = 0;
        if ((t == ACL_USER || t == ACL_GROUP) && is_user == (t == ACL_USER)) {
            qual = *(id_t *) acl_get_qualifier(e);
            hit = (qual == want_id);
        }
        if (hit) {
            acl_permset_t ps;
            acl_get_permset(e, &ps);
            char raw[8], m[8];
            perms_str(ps, raw);
            printf("%s%d 原始权限=%s", tag_name(t), (int) qual, raw);
            if (have_mask && mask_ps) {
                perms_str(mask_ps, m);
                char eff[4];
                for (int i = 0; i < 3; i++)
                    eff[i] = (raw[i] != '-' && m[i] != '-') ? raw[i] : '-';
                eff[3] = '\0';
                printf("  mask=%s  有效权限=%s", m, eff);
            }
            printf("\n");
            found = 1;
        }
    }

    if (!found)
        printf("没有 %s=%d 的专属条目：该身份经 §17.2 算法落入%s判定\n",
               is_user ? "uid" : "gid", (int) want_id,
               is_user ? "组类扫描或 other" : "组类扫描或 other");

    acl_free(acl);
    return EXIT_SUCCESS;
}
