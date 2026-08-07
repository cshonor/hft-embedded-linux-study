/*
 * CSAPP Ch11 · 网络编程 — C++ 版 (RAII socket + std::thread)
 *
 * 对照笔记:
 *   chapter-11/notes/section-11.4-套接字接口.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch11_echo_cpp ch11-network-echo.cpp -lpthread
 * 运行:
 *   终端1: ./ch11_echo_cpp
 *   终端2: nc 127.0.0.1 8080
 *
 * C++ 差异:
 *   - RAII: SocketFd 析构自动 close
 *   - std::thread 多线程 echo (每个连接一个线程)
 *   - 异常: bind/listen/accept 失败抛异常
 *   - std::atomic 控制运行标志
 */

#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <errno.h>

static constexpr int PORT  = 8080;
static constexpr int BUFSZ = 4096;

static std::atomic<bool> running{true};

// ---------- RAII: Socket 文件描述符 ----------
class SocketFd {
    int fd_;
public:
    explicit SocketFd(int fd) : fd_(fd) {
        if (fd_ < 0) throw std::runtime_error("invalid fd");
    }
    ~SocketFd() { if (fd_ >= 0) close(fd_); }

    SocketFd(const SocketFd&) = delete;
    SocketFd& operator=(const SocketFd&) = delete;
    SocketFd(SocketFd&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    SocketFd& operator=(SocketFd&& o) noexcept {
        if (this != &o) { if (fd_ >= 0) close(fd_); fd_ = o.fd_; o.fd_ = -1; }
        return *this;
    }

    int get() const { return fd_; }
    int release() { int t = fd_; fd_ = -1; return t; }
};

// ---------- 工具: setsockopt ----------
static void set_tcp_nodelay(int fd)
{
    int flag = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == 0)
        printf("  [conn %d] TCP_NODELAY=ON (Nagle 禁用)\n", fd);
}

static void set_reuseaddr(int fd)
{
    int flag = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

// ---------- echo 线程 ----------
static void echo_thread(SocketFd client)
{
    int fd = client.get();
    char buf[BUFSZ];
    ssize_t n;

    set_tcp_nodelay(fd);
    printf("  [conn %d] 线程启动\n", fd);

    while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t s = send(fd, buf + sent, n - sent, 0);
            if (s <= 0) break;
            sent += s;
        }
        buf[n] = '\0';
        printf("  [conn %d] recv %zd: %s", fd, n, buf);
    }
    printf("  [conn %d] 关闭\n", fd);
} // SocketFd 析构自动 close

// ------------------------------------------------------------------
int main()
{
    signal(SIGPIPE, SIG_IGN);

    try {
        SocketFd listen_fd(socket(AF_INET, SOCK_STREAM, 0));
        set_reuseaddr(listen_fd.get());

        struct sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(listen_fd.get(), (struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("bind failed: " + std::string(strerror(errno)));

        if (listen(listen_fd.get(), 5) < 0)
            throw std::runtime_error("listen failed");

        printf("=== CSAPP Ch11 · Echo Server C++ (port %d) ===\n\n", PORT);
        printf("  测试: nc 127.0.0.1 %d\n", PORT);
        printf("  Ctrl+C 退出\n\n");

        std::vector<std::thread> threads;

        while (running) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);

            int cfd = accept(listen_fd.get(),
                             (struct sockaddr*)&cli_addr, &cli_len);
            if (cfd < 0) {
                if (errno == EINTR) break;
                continue;
            }

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
            printf("  新连接: %s:%d\n", ip, ntohs(cli_addr.sin_port));

            // 每连接一个线程 (CSAPP 预线程化风格)
            threads.emplace_back(echo_thread, SocketFd(cfd));

            // 清理已结束的线程 (简化: detach 也可)
            threads.back().detach();
        }
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }

    printf("\n  服务器关闭\n");
    return 0;
}
