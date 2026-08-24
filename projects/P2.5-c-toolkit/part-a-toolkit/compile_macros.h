#ifndef COMPILE_MACROS_H
#define COMPILE_MACROS_H

#define BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#endif
