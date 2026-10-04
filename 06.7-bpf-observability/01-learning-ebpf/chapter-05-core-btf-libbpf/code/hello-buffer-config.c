/* hello-buffer-config.c — CO-RE 用户态 loader 最小版（ch05 §5.3 块10 补全）
 *
 * 流程: open（解析 ELF）→ load（装入内核 + CO-RE 修复）→ attach（按 SEC 自动附加）
 *       → 写 my_config 配置 → perf_buffer 收事件
 *
 * 编译见 scripts/build-core.sh（依赖 hello-buffer-config.skel.h）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "hello-buffer-config.skel.h"
#include "hello-buffer-config.h"

static volatile sig_atomic_t stop;

static void sig_handler(int sig) { (void)sig; stop = 1; }

/* perf buffer 回调：事件到达即打印 */
static void handle_event(void *ctx, int cpu, void *data, __u32 size)
{
    const struct data_t *d = data;
    (void)ctx; (void)size;
    printf("%-8d %-6d %-16s %-12s %s\n",
           d->pid, d->uid, d->command, d->message, d->path);
}

int main(int argc, char **argv)
{
    struct hello_buffer_config_bpf *skel;
    struct perf_buffer *pb;
    int err;

    /* open: 解析 ELF；load: 装入内核 + CO-RE 修复（relocate 字段偏移） */
    skel = hello_buffer_config_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "open_and_load 失败（内核无 BTF？见 scripts/check-btf.sh）\n");
        return 1;
    }

    /* attach: 按 SEC("ksyscall/execve") 自动附加 */
    err = hello_buffer_config_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "attach 失败: %d\n", err);
        goto cleanup;
    }

    /* 写配置：给 root(0) 自定义消息（演示 my_config map 的用户态写入） */
    struct user_msg_t cfg = { .message = "Hi root!" };
    __u32 uid0 = 0;
    bpf_map__update_elem(skel->maps.my_config, &uid0, sizeof(uid0),
                         &cfg, sizeof(cfg), BPF_ANY);

    /* perf buffer 收事件 */
    pb = perf_buffer__new(bpf_map__fd(skel->maps.output), 8 /* pages */,
                          handle_event, NULL, NULL, NULL);
    if (!pb) {
        fprintf(stderr, "perf_buffer__new 失败\n");
        err = 1;
        goto cleanup;
    }

    signal(SIGINT, sig_handler);
    printf("%-8s %-6s %-16s %-12s %s\n", "PID", "UID", "COMM", "MSG", "PATH");
    printf("（另开终端执行命令触发 execve；root 触发会显示 Hi root!）Ctrl-C 退出\n");
    while (!stop && perf_buffer__poll(pb, 200) >= 0)
        ;

    perf_buffer__free(pb);
cleanup:
    hello_buffer_config_bpf__destroy(skel);
    return err;
}
