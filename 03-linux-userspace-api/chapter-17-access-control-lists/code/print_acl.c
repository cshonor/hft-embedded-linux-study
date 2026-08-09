/* Dump Access ACL (mini getfacl).
 * cc -Wall -Wextra -o print_acl print_acl.c -lacl
 * ./print_acl PATH
 */
#include <acl/libacl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/acl.h>
#include <sys/types.h>
#include <unistd.h>

static void print_perms(acl_permset_t perms)
{
    int r = acl_get_perm(perms, ACL_READ);
    int w = acl_get_perm(perms, ACL_WRITE);
    int x = acl_get_perm(perms, ACL_EXECUTE);
    putchar(r == 1 ? 'r' : '-');
    putchar(w == 1 ? 'w' : '-');
    putchar(x == 1 ? 'x' : '-');
}

static void print_entry(acl_entry_t ent)
{
    acl_tag_t tag;
    acl_permset_t perms;
    void *qual;
    uid_t *uid;
    gid_t *gid;
    struct passwd *pw;
    struct group *gr;

    if (acl_get_tag_type(ent, &tag) == -1 ||
        acl_get_permset(ent, &perms) == -1) {
        perror("acl_get_*");
        return;
    }

    switch (tag) {
    case ACL_USER_OBJ:
        printf("user::");
        break;
    case ACL_USER:
        qual = acl_get_qualifier(ent);
        if (qual == NULL) {
            perror("acl_get_qualifier");
            return;
        }
        uid = qual;
        pw = getpwuid(*uid);
        printf("user:%s:", pw ? pw->pw_name : "?");
        acl_free(qual);
        break;
    case ACL_GROUP_OBJ:
        printf("group::");
        break;
    case ACL_GROUP:
        qual = acl_get_qualifier(ent);
        if (qual == NULL) {
            perror("acl_get_qualifier");
            return;
        }
        gid = qual;
        gr = getgrgid(*gid);
        printf("group:%s:", gr ? gr->gr_name : "?");
        acl_free(qual);
        break;
    case ACL_MASK:
        printf("mask::");
        break;
    case ACL_OTHER:
        printf("other::");
        break;
    default:
        printf("?:");
        break;
    }
    print_perms(perms);
    putchar('\n');
}

int main(int argc, char *argv[])
{
    acl_t acl;
    acl_entry_t ent;
    int entry_id;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PATH\n", argv[0]);
        return EXIT_FAILURE;
    }

    acl = acl_get_file(argv[1], ACL_TYPE_ACCESS);
    if (acl == NULL) {
        perror("acl_get_file");
        return EXIT_FAILURE;
    }

    printf("# file: %s\n# Access ACL:\n", argv[1]);
    for (entry_id = ACL_FIRST_ENTRY; ; entry_id = ACL_NEXT_ENTRY) {
        int rc = acl_get_entry(acl, entry_id, &ent);
        if (rc == 0)
            break;
        if (rc == -1) {
            perror("acl_get_entry");
            acl_free(acl);
            return EXIT_FAILURE;
        }
        print_entry(ent);
    }

    acl_free(acl);
    return 0;
}
