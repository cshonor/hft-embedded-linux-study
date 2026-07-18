#include <stdio.h>

typedef struct Device Device;

typedef void (*ShowFn)(Device *self);

struct Device {
    ShowFn show;
};

typedef struct {
    Device base;
    int id;
} ConsoleDev;

static void console_show(Device *self)
{
    ConsoleDev *c = (ConsoleDev *)self;
    printf("C ops table: console id=%d\n", c->id);
}

int main(void)
{
    ConsoleDev dev;
    dev.base.show = console_show;
    dev.id = 42;
    dev.base.show((Device *)&dev);
    return 0;
}
