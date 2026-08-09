#include "plugin_api.h"
#include <stdio.h>

__attribute__((constructor))
static void plugin_ctor(void)
{
    fprintf(stderr, "[plugin] constructor\n");
}

__attribute__((destructor))
static void plugin_dtor(void)
{
    fprintf(stderr, "[plugin] destructor\n");
}

static int plugin_run(int x)
{
    return x * 2;
}

static const char *plugin_name(void)
{
    return "demo-plugin";
}

static const PluginApi api = {
    .version = 1,
    .run = plugin_run,
    .name = plugin_name,
};

__attribute__((visibility("default")))
const PluginApi *plugin_get_api(void)
{
    return &api;
}
