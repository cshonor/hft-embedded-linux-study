#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <utility>

/* 析构关 fd，避免漏 close。这就是 Part B 相对 C 版要练的那一点。 */
class Fd {
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    ~Fd() { close_if(); }
    Fd(const Fd &) = delete;
    Fd &operator=(const Fd &) = delete;
    Fd(Fd &&o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    Fd &operator=(Fd &&o) noexcept
    {
        if (this != &o) {
            close_if();
            fd_ = o.fd_;
            o.fd_ = -1;
        }
        return *this;
    }
    int get() const { return fd_; }

private:
    void close_if()
    {
        if (fd_ >= 0)
            ::close(fd_);
        fd_ = -1;
    }
    int fd_;
};

static Fd listen_loopback(uint16_t port)
{
    int raw = ::socket(AF_INET, SOCK_STREAM, 0);
    Fd fd(raw);
    int yes = 1;
    setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(fd.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0 ||
        listen(fd.get(), 16) < 0) {
        return Fd{-1};
    }
    return fd;
}

static uint16_t bound_port(int fd)
{
    sockaddr_in addr{};
    socklen_t len = sizeof addr;
    getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len);
    return ntohs(addr.sin_port);
}

static int self_test()
{
    Fd lst = listen_loopback(0);
    if (lst.get() < 0)
        return 1;
    uint16_t port = bound_port(lst.get());
    pid_t pid = fork();
    if (pid == 0) {
        Fd c(::socket(AF_INET, SOCK_STREAM, 0));
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(port);
        if (connect(c.get(), reinterpret_cast<sockaddr *>(&a), sizeof a) < 0)
            _exit(2);
        const char msg[] = "ping-p3cpp";
        write(c.get(), msg, sizeof msg - 1);
        shutdown(c.get(), SHUT_WR);
        char buf[64];
        ssize_t n = read(c.get(), buf, sizeof buf);
        _exit(n == (ssize_t)(sizeof msg - 1) && memcmp(buf, msg, (size_t)n) == 0 ? 0 : 3);
    }
    Fd conn(accept(lst.get(), nullptr, nullptr));
    char buf[1024];
    ssize_t n;
    while ((n = read(conn.get(), buf, sizeof buf)) > 0)
        write(conn.get(), buf, static_cast<size_t>(n));
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
        std::cout << "part-b-cpp-rewrite: RAII echo self-test OK\n";
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && std::string(argv[1]) == "--self-test")
        return self_test();

    Fd lst = listen_loopback(8081);
    if (lst.get() < 0) {
        perror("listen");
        return 1;
    }
    std::cout << "C++ echo_server on 8081\n";
    for (;;) {
        Fd conn(accept(lst.get(), nullptr, nullptr));
        if (conn.get() < 0)
            continue;
        char buf[1024];
        ssize_t n;
        while ((n = read(conn.get(), buf, sizeof buf)) > 0)
            write(conn.get(), buf, static_cast<size_t>(n));
    }
}
