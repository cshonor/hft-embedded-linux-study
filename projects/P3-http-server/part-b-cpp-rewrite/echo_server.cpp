#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
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

static std::optional<std::string_view> parse_get_path(std::string_view req)
{
    constexpr std::string_view prefix{"GET "};
    if (req.size() < prefix.size() || req.substr(0, prefix.size()) != prefix)
        return std::nullopt;
    auto rest = req.substr(prefix.size());
    while (!rest.empty() && rest.front() == ' ')
        rest.remove_prefix(1);
    auto end = rest.find_first_of(" \r\n");
    auto path = rest.substr(0, end);
    if (path.empty())
        return std::nullopt;
    return path;
}

static void send_http(int fd, int code, std::string_view body)
{
    std::string hdr = "HTTP/1.1 " + std::to_string(code) + (code == 200 ? " OK" : " Not Found");
    hdr += "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";
    write(fd, hdr.data(), hdr.size());
    write(fd, body.data(), body.size());
}

static void handle_conn(int conn)
{
    char buf[2048];
    ssize_t n = read(conn, buf, sizeof buf);
    if (n <= 0)
        return;
    std::string_view req{buf, static_cast<size_t>(n)};
    if (auto path = parse_get_path(req)) {
        if (*path == "/" || *path == "/hello.txt")
            send_http(conn, 200, "p3-hello\n");
        else
            send_http(conn, 404, "missing\n");
        return;
    }
    write(conn, buf, static_cast<size_t>(n));
}

static int client_expect(uint16_t port, const char *req, const char *needle)
{
    Fd c(::socket(AF_INET, SOCK_STREAM, 0));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(port);
    if (connect(c.get(), reinterpret_cast<sockaddr *>(&a), sizeof a) < 0)
        return 2;
    write(c.get(), req, strlen(req));
    shutdown(c.get(), SHUT_WR);
    char buf[512];
    ssize_t n = read(c.get(), buf, sizeof buf - 1);
    if (n <= 0)
        return 3;
    buf[n] = '\0';
    return std::strstr(buf, needle) ? 0 : 4;
}

static int self_test()
{
    Fd lst = listen_loopback(0);
    if (lst.get() < 0)
        return 1;
    uint16_t port = bound_port(lst.get());
    pid_t pid = fork();
    if (pid == 0) {
        int e1 = client_expect(port, "GET /hello.txt HTTP/1.1\r\n\r\n", "p3-hello");
        int e2 = client_expect(port, "GET /nope HTTP/1.1\r\n\r\n", "HTTP/1.1 404");
        _exit(e1 || e2);
    }
    for (int i = 0; i < 2; i++) {
        Fd conn(accept(lst.get(), nullptr, nullptr));
        if (conn.get() < 0)
            continue;
        handle_conn(conn.get());
    }
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
        std::cout << "part-b-cpp-rewrite: GET string_view self-test OK\n";
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
        handle_conn(conn.get());
    }
}
