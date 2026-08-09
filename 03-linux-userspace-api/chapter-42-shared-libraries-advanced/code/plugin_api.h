#ifndef PLUGIN_API_H
#define PLUGIN_API_H

typedef struct {
    int version;
    int (*run)(int x);
    const char *(*name)(void);
} PluginApi;

/* Host dlsym's this single symbol (extern "C" if built as C++). */
const PluginApi *plugin_get_api(void);

#endif
