#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>

/* 从成员指针减偏移，得到宿主结构体。必须用 (char *)，否则按元素步长减会算错。 */
#define container_of_simple(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define container_of(ptr, type, member)                                           \
    ({                                                                            \
        const typeof(((type *)0)->member) *__mptr = (ptr);                        \
        (type *)((char *)__mptr - offsetof(type, member));                        \
    })

#endif
