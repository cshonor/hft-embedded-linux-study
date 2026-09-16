#ifndef SYSMACROS_SHIM_H
#define SYSMACROS_SHIM_H
/* macOS 没有 <sys/sysmacros.h>；major/minor 在 sys/types.h。空壳只为让原书 t_stat.c 零改动编译。 */
#include <sys/types.h>
#endif
