#include "container_of.h"

#include <stdio.h>

struct node {
    int id;
    struct node *next;
};

int main(void)
{
    struct node n = {.id = 42, .next = NULL};
    int *pid = &n.id;
    struct node *back = container_of(pid, struct node, id);
    printf("container_of ok: id=%d\n", back->id);
    return 0;
}
