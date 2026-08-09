#include <stdio.h>
#include <string.h>

typedef void (*cmd_fn)(void);

static void cmd_hello(void) { puts("hello"); }
static void cmd_bye(void) { puts("bye"); }

struct entry {
    const char *name;
    cmd_fn fn;
};

static const struct entry table[] = {
    {"hello", cmd_hello},
    {"bye", cmd_bye},
};

int main(void)
{
    const char *name = "hello";
    for (size_t i = 0; i < sizeof table / sizeof table[0]; i++) {
        if (strcmp(table[i].name, name) == 0) {
            table[i].fn();
            return 0;
        }
    }
    puts("not found");
    return 1;
}
