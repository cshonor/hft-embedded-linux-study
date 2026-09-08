#ifndef DEMO_CFG_H
#define DEMO_CFG_H

#if DEBUG
static const char *mode = "debug (#if DEBUG nonzero)";
#else
static const char *mode = "release (#if DEBUG is 0 or undefined)";
#endif

#endif
