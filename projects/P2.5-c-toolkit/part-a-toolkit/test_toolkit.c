#include "compile_macros.h"
#include "container_of.h"
#include "list.h"
#include "ringbuf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct student {
    int id;
    struct list_head list;
};

struct node {
    int id;
    char pad;
};

static void test_container_of(void)
{
    struct node n = {.id = 7, .pad = 'x'};
    struct node *back = container_of(&n.id, struct node, id);
    assert(back == &n);
}

static void test_list(void)
{
    LIST_HEAD(head);
    struct student a = {.id = 1}, b = {.id = 2};
    list_init(&a.list);
    list_init(&b.list);
    list_add(&a.list, &head);
    list_add(&b.list, &head);

    int seen = 0;
    struct student *pos;
    list_for_each_entry(pos, &head, list) {
        seen += pos->id;
    }
    assert(seen == 3);
    list_del(&a.list);
}

static void test_ring(void)
{
    struct ringbuf rb;
    assert(ringbuf_init(&rb, 8, sizeof(int)) == 0);
    for (int i = 0; i < 7; i++)
        assert(ringbuf_push(&rb, &i) == 0);
    assert(ringbuf_push(&rb, &(int){99}) == -1);
    int v;
    assert(ringbuf_pop(&rb, &v) == 0 && v == 0);
    ringbuf_destroy(&rb);
}

static void test_macros(void)
{
    int arr[10];
    assert(ARRAY_SIZE(arr) == 10);
    BUILD_BUG_ON(sizeof(char) != 1);
    int x = 1;
    assert(__same_type(x, arr[0]));
    assert(likely(x == 1));
    assert(!unlikely(x == 0));
}

int main(void)
{
    test_container_of();
    test_list();
    test_ring();
    test_macros();
    puts("part-a-toolkit: OK");
    return 0;
}
