#ifndef SYSMACROS_SHIM_H
#define SYSMACROS_SHIM_H
/* macOS 没有 <sys/sysmacros.h>；major/minor 在 sys/types.h。本 shim 只为让原书
 * t_stat.c 零改动编译。
 *
 * ⚠ Linux 下绝不能让本文件遮蔽系统头：带 -I. 编译时 <sys/sysmacros.h> 会优先
 * 解析到本文件，空壳会导致 major/minor 链接失败（2026-10 Ubuntu 24.04 实测）。
 * 因此非 Apple 平台用 #include_next 透传到真正的系统头。 */
#if defined(__APPLE__)
#include <sys/types.h>
#else
#include_next <sys/sysmacros.h>
#endif
#endif
