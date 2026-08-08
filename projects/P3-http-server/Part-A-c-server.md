# P3 Part A — C 版并发 HTTP Server

> 用 C + epoll + 线程池从零写一个能压测的 HTTP server。
> **做法：项目驱动，[`04`](../../04-linux-userspace-api/) / [`15`](../../15-network-sockets/) 笔记当字典。**

---

## 最小预备

翻一眼标题，知道"有这么个东西"就行，不用读懂细节：

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [TLPI ch63 epoll](../../04-linux-userspace-api/chapter-63-alternative-io/) | epoll = 替代 select/poll 的高性能 I/O 多路复用 |
| [TLPI ch56 socket 入门](../../04-linux-userspace-api/chapter-56-sockets-intro/) | socket/bind/listen/accept 四件套 |
| [TLPI ch29 线程](../../04-linux-userspace-api/chapter-29-threads-intro/) | pthread_create/join 基础 |
| [PNP epoll 实战](../../15-network-sockets/muduo-sockets/code/07_IO_epoll/notes.md) | epoll LT vs ET 实际代码 |
| [CSAPP 12.2 I/O 多路复用](../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.2-基于I-O多路复用的并发编程.md) | 为什么要多路复用 |

---

## Phase 1：单线程 epoll echo server（30 分钟）

### 做什么

最小的能跑的 server：收一个连接，echo 回去。先别想 HTTP，先让 epoll 跑起来。

### 代码骨架

```c
// src/echo_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PORT 8080
#define MAX_EVENTS 64
#define BUF_SIZE 4096

// 设非阻塞
static int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    // 1. 创建 listen socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, SOMAXCONN);
    set_nonblock(listen_fd);

    // 2. 创建 epoll
    int epfd = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = listen_fd };
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    // 3. 事件循环
    struct epoll_event events[MAX_EVENTS];
    char buf[BUF_SIZE];
    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                // 新连接
                int conn_fd = accept(listen_fd, NULL, NULL);
                set_nonblock(conn_fd);
                struct epoll_event cev = { .events = EPOLLIN | EPOLLET, .data.fd = conn_fd };
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &cev);
            } else {
                // 已有连接：读 + echo
                int fd = events[i].data.fd;
                ssize_t r = read(fd, buf, sizeof(buf));
                if (r <= 0) { close(fd); epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); }
                else write(fd, buf, r);  // echo 回去
            }
        }
    }
}
```

### 分步实现

1. **socket + bind + listen**：这三步固定写法，先让 `telnet localhost 8080` 能连上
2. **加 epoll**：listen_fd 加入 epoll，事件循环 `epoll_wait`
3. **accept 新连接**：`EPOLLIN` on listen_fd = 有新连接，`accept` 后加入 epoll
4. **echo 已有连接**：`read` 后 `write` 回去；`r <= 0` 时关连接
5. **加 `EPOLLET`（边沿触发）**：ET 模式下必须循环 `read` 到 `EAGAIN`，否则会丢数据

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 忘了 `set_nonblock` | `read` 卡死 | ET 模式 + 阻塞 fd = 死锁 |
| ET 模式不循环读 | 丢数据 | ET 只通知一次，必须读到 EAGAIN |
| 忘了 `SO_REUSEADDR` | 重启 bind 失败 | TIME_WAIT 占着端口 |
| accept 返回 EAGAIN 不处理 | 空转 100% CPU | ET 模式 accept 也要循环 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| socket/bind/listen 流程 | [TLPI ch56](../../04-linux-userspace-api/chapter-56-sockets-intro/) |
| epoll API | [TLPI ch63](../../04-linux-userspace-api/chapter-63-alternative-io/) · [PNP epoll](../../15-network-sockets/muduo-sockets/code/07_IO_epoll/notes.md) |
| LT vs ET 区别 | [CSAPP 12.2](../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.2-基于I-O多路复用的并发编程.md) |
| 非阻塞 I/O | [PNP NonBlockingIO](../../15-network-sockets/muduo-sockets/code/06_NonBlockingIO/notes.md) |

### 测试

```bash
gcc -o echo_server src/echo_server.c && ./echo_server &
# 终端 2
telnet localhost 8080
# 输入什么就回什么 → 成功
```

---

## Phase 2：HTTP 请求解析（1 小时）

### 做什么

在 echo server 基础上加 HTTP/1.1 请求解析：解析请求行 + headers，返回简单响应。

### 代码骨架

```c
// 简化的 HTTP 请求结构
struct http_request {
    char method[8];     // "GET" / "POST"
    char path[256];     // "/index.html"
    char host[128];     // "localhost:8080"
    int  content_length;
};

// 解析请求行：GET /path HTTP/1.1\r\n
int parse_request_line(const char *line, struct http_request *req) {
    // sscanf 或手动 strtok
    return sscanf(line, "%7s %255s %*s", req->method, req->path) == 2;
}

// 解析 header：Host: localhost:8080\r\n
void parse_header(const char *line, struct http_request *req) {
    if (strncmp(line, "Host:", 5) == 0)
        sscanf(line + 5, " %127s", req->host);
    else if (strncmp(line, "Content-Length:", 15) == 0)
        req->content_length = atoi(line + 15);
}

// 返回 HTTP 响应
void send_response(int fd, int status, const char *content_type, const char *body) {
    char header[512];
    int len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n", status, content_type, strlen(body));
    write(fd, header, len);
    write(fd, body, strlen(body));
}
```

### 分步实现

1. **读完整请求**：HTTP 请求以 `\r\n\r\n` 结束，需要缓冲直到看到这个标记
2. **拆行解析**：`strstr(buf, "\r\n\r\n")` 找到 headers 结束位置，然后逐行解析
3. **返回响应**：`HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<body>`
4. **用 curl 测试**：`curl http://localhost:8080/`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 请求没读完 | 解析到半个请求 | TCP 是流，一次 read 可能只到半个 header |
| Content-Length 没处理 POST body | POST 数据丢 | headers 后面还有 body |
| 固定大小缓冲区溢出 | 段错误 | 恶意请求发超长 path/header |

---

## Phase 3：线程池（1-2 小时）

### 做什么

主线程只做 epoll + accept，把已连接 fd 丢给线程池处理。

### 代码骨架

```c
// 线程池
struct threadpool {
    pthread_t *threads;
    int num_threads;
    // 任务队列
    int *task_queue;        // 存 fd
    int queue_head, queue_tail, queue_size;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    int shutdown;
};

// worker 线程：等任务 → 取 fd → 处理
void *worker(void *arg) {
    struct threadpool *pool = arg;
    for (;;) {
        pthread_mutex_lock(&pool->lock);
        while (pool->queue_head == pool->queue_tail && !pool->shutdown)
            pthread_cond_wait(&pool->notify, &pool->lock);
        if (pool->shutdown) { pthread_mutex_unlock(&pool->lock); return NULL; }
        int fd = pool->task_queue[pool->queue_head % pool->queue_size];
        pool->queue_head++;
        pthread_mutex_unlock(&pool->lock);

        // 处理这个连接（解析 HTTP + 返回响应）
        handle_connection(fd);
        close(fd);
    }
}

// 主线程：accept 后丢进队列
void submit_task(struct threadpool *pool, int fd) {
    pthread_mutex_lock(&pool->lock);
    pool->task_queue[pool->queue_tail % pool->queue_size] = fd;
    pool->queue_tail++;
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
}
```

### 分步实现

1. **先写线程池**：任务队列（环形数组）+ mutex + condition variable + N 个 worker
2. **worker 逻辑**：`pthread_cond_wait` 等任务 → 取 fd → `handle_connection(fd)` → `close(fd)`
3. **主线程改造**：epoll 只监听 listen_fd，accept 后 `submit_task(pool, conn_fd)`
4. **注意**：fd 从主线程传到 worker 线程后，不能再在主线程的 epoll 里操作它

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 主线程和 worker 同时操作 fd | 数据错乱 | accept 后要么主线程处理，要么丢给 worker，不能两边都碰 |
| 条件变量用错 | 死锁或丢唤醒 | `while` 不是 `if`（spurious wakeup） |
| 队列满了 | 丢任务 | 环形队列满判断 + 丢弃策略 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| pthread 基础 | [TLPI ch29](../../04-linux-userspace-api/chapter-29-threads-intro/) |
| mutex/cond | [TLPI ch30](../../04-linux-userspace-api/chapter-30-thread-synchronization/) |
| 条件变量为什么用 while | [CSAPP 12.5](../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md) |

---

## Phase 4：静态文件服务 + 压测（1 小时）

### 做什么

加文件读取（`open/read` 或 `sendfile` 零拷贝），MIME 类型映射，用 `ab`/`wrk` 压测。

### 分步实现

1. **路由**：`GET /` → `index.html`，`GET /xxx` → 文件路径
2. **MIME 映射**：`.html` → `text/html`，`.css` → `text/css`，`.js` → `application/javascript`
3. **`sendfile` 零拷贝**：`sendfile(conn_fd, file_fd, NULL, file_size)` — 内核直接从 page cache 发到 socket，不经过用户态
4. **压测**：`ab -n 10000 -c 100 http://localhost:8080/`，看 RPS 和延迟

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| sendfile 零拷贝 | [TLPI ch61](../../04-linux-userspace-api/chapter-61-sockets-advanced/) |
| 文件 I/O | [TLPI ch04](../../04-linux-userspace-api/chapter-04-file-io-universal/) |

---

## 测试验证

```bash
# Phase 1
telnet localhost 8080    # echo 回来

# Phase 2
curl -v http://localhost:8080/

# Phase 4
ab -n 10000 -c 100 http://localhost:8080/index.html
wrk -t4 -c100 -d10s http://localhost:8080/
```

← [P3 索引](./README.md) · [04 模块](../../04-linux-userspace-api/)
