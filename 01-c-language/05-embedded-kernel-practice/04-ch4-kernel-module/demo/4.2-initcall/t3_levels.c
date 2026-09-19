/* T3: 模拟 initcall 级别 —— 8 个级别段（0/1/2/3/4/5/6/7），按级别顺序执行
 * 对应 init.h 的 pure_initcall/core_initcall/.../late_initcall，
 * 以及 init/main.c 的 initcall_levels[] + do_initcall_level()。
 */
#include <stdio.h>

typedef int (*initcall_t)(void);

extern initcall_t __initcall0_start[];
extern initcall_t __initcall1_start[];
extern initcall_t __initcall2_start[];
extern initcall_t __initcall3_start[];
extern initcall_t __initcall4_start[];
extern initcall_t __initcall5_start[];
extern initcall_t __initcall6_start[];
extern initcall_t __initcall7_start[];
extern initcall_t __initcall_end[];

#define SIM_LEVEL(fn, lv) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall" #lv ".init"))) \
        __sim_##fn = fn

static int pure_l0(void)   { printf("  [pure]   ");      return 0; }
static int core_l1(void)   { printf("  [core]   ");      return 0; }
static int post_l2(void)   { printf("  [post]   ");      return 0; }
static int arch_l3(void)   { printf("  [arch]   ");      return 0; }
static int subsys_l4(void) { printf("  [subsys] ");      return 0; }
static int fs_l5(void)     { printf("  [fs]     ");      return 0; }
static int dev_l6(void)    { printf("  [device] ");     return 0; }
static int late_l7(void)   { printf("  [late]   ");      return 0; }

SIM_LEVEL(pure_l0, 0);
SIM_LEVEL(core_l1, 1);
SIM_LEVEL(post_l2, 2);
SIM_LEVEL(arch_l3, 3);
SIM_LEVEL(subsys_l4, 4);
SIM_LEVEL(fs_l5, 5);
SIM_LEVEL(dev_l6, 6);
SIM_LEVEL(late_l7, 7);

static const char *level_names[] = {
    "pure", "core", "postcore", "arch",
    "subsys", "fs", "device", "late"
};

static initcall_t *initcall_levels[] = {
    __initcall0_start, __initcall1_start, __initcall2_start, __initcall3_start,
    __initcall4_start, __initcall5_start, __initcall6_start, __initcall7_start,
    __initcall_end,
};

static void do_initcall_level(int level)
{
    initcall_t *fn;
    printf("level %d (%s):\n", level, level_names[level]);
    for (fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++) {
        int ret = (*fn)();
        printf("    ret=%d\n", ret);
    }
}

static void do_initcalls(void)
{
    int level;
    for (level = 0; level < 8; level++)
        do_initcall_level(level);
}

int main(void)
{
    printf("T3: initcall levels (0-7)\n");
    do_initcalls();
    return 0;
}
