/*
 * T3: .modinfo 段收集 + vermagic 比对
 *
 * 内核模块用 MODULE_INFO(tag, info) 把 "tag=info" 字符串放进 .modinfo 段：
 *   #define MODULE_INFO(tag, info) __MODULE_INFO(tag, tag, info)
 *   → static const char __module_param_##tag[]
 *       __attribute__((section(".modinfo"), used, aligned(1))) = "tag=info"
 *
 * modpost 生成的 .mod.c 还会放：
 *   MODULE_INFO(vermagic, VERMAGIC_STRING);   ← 版本印章
 *   MODULE_INFO(name, KBUILD_MODNAME);
 *
 * 加载时 get_modinfo(info, "vermagic") 在 .modinfo 里线性扫描 "vermagic=..."
 * 然后跟内核自己的 VERMAGIC_STRING 比对：不一致 → -ENOEXEC（拒绝加载）。
 *
 * 本 demo 用 section 名 "modinfo"（无点，GNU ld 才能自动给边界符号），
 * 效果与内核 .modinfo 完全一致。
 */
#include <stdio.h>
#include <string.h>

/* 模拟 VERMAGIC_STRING：内核版本 + SMP + preempt + modversions 等拼出来的串 */
#define KERNEL_VERMAGIC "6.6.0 SMP preempt mod_unload modversions"

/* MOD_INFO(tag, val) → 把 "tag=val\0" 放进 modinfo 段 */
#define MOD_INFO(tag, val) \
    static const char __modinfo_##tag[] \
        __attribute__((used, section("modinfo"), aligned(1))) = #tag "=" val

/* 一个“模块”用 modpost 生成的元信息 */
MOD_INFO(vermagic,    "6.6.0 SMP preempt mod_unload modversions"); /* 匹配 */
MOD_INFO(name,        "mymod");
MOD_INFO(author,      "wzp");
MOD_INFO(license,     "GPL");
MOD_INFO(description, "a tiny demo module");
MOD_INFO(version,     "1.0");
MOD_INFO(intree,      "Y");

/* GNU ld 自动提供的边界符号（section 名是合法 C 标识符时） */
extern const char __start_modinfo[];
extern const char __stop_modinfo[];

/* 在 .modinfo 里线性扫描 "tag=..."，返回值的指针（对应 get_modinfo） */
static const char *get_modinfo(const char *tag)
{
    size_t taglen = strlen(tag);
    for (const char *p = __start_modinfo; p < __stop_modinfo; ) {
        /* 每个 entry 是一个 NUL 结尾的 "tag=value" 串 */
        size_t entry_len = strlen(p);
        if (entry_len == 0) { p++; continue; }
        if (strncmp(p, tag, taglen) == 0 && p[taglen] == '=')
            return p + taglen + 1;
        p += entry_len + 1;
    }
    return NULL;
}

int main(void)
{
    printf("=== T3: .modinfo 段收集 + vermagic 比对 ===\n\n");

    printf("--- 原始 .modinfo 段内容（按 entry 遍历） ---\n");
    for (const char *p = __start_modinfo; p < __stop_modinfo; ) {
        if (*p == '\0') { p++; continue; }
        printf("  %s\n", p);
        p += strlen(p) + 1;
    }

    printf("\n--- 用 get_modinfo 取出各项 ---\n");
    const char *tags[] = {"vermagic","name","author","license",
                          "description","version","intree","missing"};
    for (size_t i = 0; i < sizeof(tags)/sizeof(tags[0]); i++) {
        const char *v = get_modinfo(tags[i]);
        printf("  %-12s = %s\n", tags[i], v ? v : "(not found)");
    }

    /* === vermagic 比对：加载门禁 === */
    printf("\n--- vermagic 门禁检查 ---\n");
    const char *mod_vmagic = get_modinfo("vermagic");
    printf("  内核 vermagic : %s\n", KERNEL_VERMAGIC);
    printf("  模块 vermagic : %s\n", mod_vmagic ? mod_vmagic : "(none)");
    if (mod_vmagic && strcmp(mod_vmagic, KERNEL_VERMAGIC) == 0)
        printf("  结果: 匹配，允许加载\n");
    else
        printf("  结果: 不匹配，-ENOEXEC 拒绝加载\n");

    /* 模拟版本不符的模块 */
    printf("\n--- 模拟一个版本不符的模块 ---\n");
    const char *bad_vmagic = "6.5.0 SMP mod_unload";
    printf("  模块 vermagic : %s\n", bad_vmagic);
    printf("  结果: %s\n",
           strcmp(bad_vmagic, KERNEL_VERMAGIC) == 0
               ? "匹配" : "不匹配，拒绝（防止 ABI 不兼容）");

    return 0;
}
