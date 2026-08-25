#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 16
#define BUFSZ 2048

static int make_listener(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(fd, BACKLOG) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static uint16_t bound_port(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
        return 0;
    return ntohs(addr.sin_port);
}

/* 只认请求行 `GET /path HTTP/1.x`。路径复制到 out，最多 outsz-1。 */
static int parse_get_path(const char *req, char *out, size_t outsz)
{
    if (strncmp(req, "GET ", 4) != 0)
        return -1;
    const char *p = req + 4;
    while (*p == ' ')
        p++;
    const char *e = p;
    while (*e && *e != ' ' && *e != '\r' && *e != '\n')
        e++;
    size_t n = (size_t)(e - p);
    if (n == 0 || n >= outsz)
        return -1;
    memcpy(out, p, n);
    out[n] = '\0';
    return 0;
}

static void send_http(int fd, int code, const char *body)
{
    char hdr[128];
    int blen = (int)strlen(body);
    const char *reason = (code == 200) ? "OK" : "Not Found";
    int hlen = snprintf(hdr, sizeof hdr,
                        "HTTP/1.1 %d %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                        code, reason, blen);
    write(fd, hdr, (size_t)hlen);
    write(fd, body, (size_t)blen);
}

static void handle_conn(int conn)
{
    char buf[BUFSZ];
    ssize_t n = read(conn, buf, sizeof buf - 1);
    if (n <= 0)
        return;
    buf[n] = '\0';

    char path[256];
    if (parse_get_path(buf, path, sizeof path) == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/hello.txt") == 0)
            send_http(conn, 200, "p3-hello\n");
        else
            send_http(conn, 404, "missing\n");
        return;
    }

    /* 非 HTTP：原样 echo，方便 nc 调试。 */
    write(conn, buf, (size_t)n);
    while ((n = read(conn, buf, sizeof buf)) > 0)
        write(conn, buf, (size_t)n);
}

static int client_expect(uint16_t port, const char *req, const char *needle)
{
    int c = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(port);
    if (connect(c, (struct sockaddr *)&a, sizeof a) < 0)
        return 2;
    write(c, req, strlen(req));
    shutdown(c, SHUT_WR);
    char buf[512];
    ssize_t n = read(c, buf, sizeof buf - 1);
    close(c);
    if (n <= 0)
        return 3;
    buf[n] = '\0';
    return strstr(buf, needle) ? 0 : 4;
}

static int self_test(void)
{
    int listen_fd = make_listener(0);
    if (listen_fd < 0) {
        perror("listen");
        return 1;
    }
    uint16_t port = bound_port(listen_fd);

    pid_t pid = fork();
    if (pid == 0) {
        close(listen_fd);
        int e1 = client_expect(port, "GET /hello.txt HTTP/1.1\r\n\r\n", "p3-hello");
        int e2 = client_expect(port, "GET /nope HTTP/1.1\r\n\r\n", "HTTP/1.1 404");
        int e3 = client_expect(port, "ping-p3", "ping-p3");
        _exit(e1 || e2 || e3);
    }

    for (int i = 0; i < 3; i++) {
        int conn = accept(listen_fd, NULL, NULL);
        if (conn < 0)
            continue;
        handle_conn(conn);
        close(conn);
    }
    close(listen_fd);
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
        puts("part-a-c-server: GET 200/404 + echo self-test OK");
        return 0;
    }
    fprintf(stderr, "self-test failed status=%d\n", st);
    return 1;
}

static void serve_forever(int listen_fd)
{
    printf("echo_server listening (Ctrl-C to stop)\n");
    for (;;) {
        int conn = accept(listen_fd, NULL, NULL);
        if (conn < 0)
            continue;
        handle_conn(conn);
        close(conn);
    }
}

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "--self-test") == 0)
        return self_test();

    int listen_fd = make_listener(PORT);
    if (listen_fd < 0) {
        perror("bind/listen");
        return 1;
    }
    printf("echo_server on %d\n", PORT);
    serve_forever(listen_fd);
    return 0;
}
