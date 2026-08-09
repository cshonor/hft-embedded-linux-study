#define _GNU_SOURCE
#include "plugin_api.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *path;
    void *handle;
    const PluginApi *(*get_api)(void);
    const PluginApi *api;
    Dl_info info;
    char *err;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <plugin.so>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    (void)dlerror();
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    err = dlerror();
    if (handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", err ? err : "(null)");
        return 1;
    }

    *(void **)&get_api = dlsym(handle, "plugin_get_api");
    err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "dlsym: %s\n", err);
        dlclose(handle);
        return 1;
    }

    api = get_api();
    printf("plugin=%s version=%d run(21)=%d\n",
           api->name(), api->version, api->run(21));

    if (dladdr((void *)api->run, &info) != 0) {
        printf("dladdr: %s in %s\n",
               info.dli_sname ? info.dli_sname : "?",
               info.dli_fname ? info.dli_fname : "?");
    }

    if (dlclose(handle) != 0)
        fprintf(stderr, "dlclose: %s\n", dlerror());

    return 0;
}
