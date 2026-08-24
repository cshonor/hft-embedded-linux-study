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
#define BUFSZ 1024

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
        int c = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in a;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(port);
        if (connect(c, (struct sockaddr *)&a, sizeof a) < 0)
            _exit(2);
        const char *msg = "ping-p3";
        write(c, msg, strlen(msg));
        shutdown(c, SHUT_WR);
        char buf[64];
        ssize_t n = read(c, buf, sizeof buf);
        close(c);
        _exit(n == (ssize_t)strlen(msg) && memcmp(buf, msg, (size_t)n) == 0 ? 0 : 3);
    }
    int conn = accept(listen_fd, NULL, NULL);
    char buf[BUFSZ];
    ssize_t n;
    while ((n = read(conn, buf, sizeof buf)) > 0)
        write(conn, buf, (size_t)n);
    close(conn);
    close(listen_fd);
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
        puts("part-a-c-server: echo self-test OK");
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
        char buf[BUFSZ];
        ssize_t n;
        while ((n = read(conn, buf, sizeof buf)) > 0) {
            /* 最小 HTTP：看见 GET 就回一行，方便 wrk/ab 以后再加。 */
            if (n >= 3 && memcmp(buf, "GET", 3) == 0) {
                const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
                write(conn, resp, strlen(resp));
                break;
            }
            if (write(conn, buf, (size_t)n) < 0)
                break;
        }
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
