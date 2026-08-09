/* Build extended Access ACL: owner/group/other + named user + MASK.
 * Grants current user rwx on the file via ACL_USER (demo).
 *
 * cc -Wall -Wextra -o set_named_user_acl set_named_user_acl.c -lacl
 * ./set_named_user_acl [/tmp/tlpi_acl_demo.txt]
 */
#include <acl/libacl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/acl.h>
#include <sys/stat.h>
#include <unistd.h>

static int set_rwx(acl_permset_t p, int r, int w, int x)
{
    if (acl_clear_perms(p) == -1)
        return -1;
    if (r && acl_add_perm(p, ACL_READ) == -1)
        return -1;
    if (w && acl_add_perm(p, ACL_WRITE) == -1)
        return -1;
    if (x && acl_add_perm(p, ACL_EXECUTE) == -1)
        return -1;
    return 0;
}

static int add_entry(acl_t *acl, acl_tag_t tag, const void *qual,
                     int r, int w, int x)
{
    acl_entry_t ent;
    acl_permset_t perms;

    if (acl_create_entry(acl, &ent) == -1)
        return -1;
    if (acl_set_tag_type(ent, tag) == -1)
        return -1;
    if (qual != NULL && acl_set_qualifier(ent, qual) == -1)
        return -1;
    if (acl_get_permset(ent, &perms) == -1)
        return -1;
    if (set_rwx(perms, r, w, x) == -1)
        return -1;
    if (acl_set_permset(ent, perms) == -1)
        return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/tlpi_acl_demo.txt";
    uid_t me = getuid();
    acl_t acl;
    int fd;

    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    close(fd);

    acl = acl_init(6);
    if (acl == NULL) {
        perror("acl_init");
        return EXIT_FAILURE;
    }

    /* Minimal base + named user + mask (extended ACL) */
    if (add_entry(&acl, ACL_USER_OBJ, NULL, 1, 1, 0) == -1 ||
        add_entry(&acl, ACL_USER, &me, 1, 1, 1) == -1 ||
        add_entry(&acl, ACL_GROUP_OBJ, NULL, 1, 0, 0) == -1 ||
        add_entry(&acl, ACL_MASK, NULL, 1, 1, 0) == -1 ||  /* r-w-: named user effective rw- */
        add_entry(&acl, ACL_OTHER, NULL, 0, 0, 0) == -1) {
        perror("build acl");
        acl_free(acl);
        return EXIT_FAILURE;
    }

    /* Do not acl_calc_mask() here: we intentionally set a tighter MASK. */

    if (acl_valid(acl) != 0) {
        fprintf(stderr, "acl_valid failed\n");
        acl_free(acl);
        return EXIT_FAILURE;
    }

    if (acl_set_file(path, ACL_TYPE_ACCESS, acl) == -1) {
        perror("acl_set_file");
        acl_free(acl);
        return EXIT_FAILURE;
    }
    acl_free(acl);

    printf("set extended ACL on %s\n", path);
    printf("  named user uid=%u wants rwx, mask is rw- => effective rw-\n",
           (unsigned)me);
    printf("  run: getfacl %s && ls -l %s\n", path, path);
    return 0;
}
