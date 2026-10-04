/* hello-buffer-config.h — 用户态/内核态共享的结构定义（ch05 §5.3 块3/4/6）
 *
 * CO-RE 程序的典型组织：结构体放共享头，.bpf.c 和 loader .c 都 include，
 * 保证两边对事件格式的理解逐字节一致。
 */
#ifndef __HELLO_BUFFER_CONFIG_H
#define __HELLO_BUFFER_CONFIG_H

#define MAX_PATH 256

/* 内核 → 用户态的事件（perf buffer 里推的东西） */
struct data_t {
    int pid;
    int uid;
    char command[16];
    char message[12];
    char path[MAX_PATH];
};

/* 用户态 → 内核的配置（HASH map：uid → 自定义消息） */
struct user_msg_t {
    char message[12];
};

#endif
