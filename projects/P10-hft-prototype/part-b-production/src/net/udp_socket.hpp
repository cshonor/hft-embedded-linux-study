#pragma once

/*
 * udp_socket.hpp — UDP 组播 socket 的 RAII 封装（Phase 3）。
 *
 * 只包最薄一层（join/leave/recv/send_to），不做线程模型——
 * 线程归引擎主循环管（run-to-completion，见 P8/P10 part-a）。
 *
 * 设计决定：
 *   - 非阻塞 recv（MSG_DONTWAIT）：feed 线程是纯 poll 循环，任何阻塞都是事故；
 *   - join 需要 interface IP（多网卡机器必须显式指定，同 03.5 UNP 的教训）；
 *   - 本类不管 IGMP 之外的组播路由问题——交换机侧静态配置（见 13-dpdk
 *     chapter-05 组播行情接入笔记的"收不到包先查 IGMP"）。
 */

#include <cerrno>
#include <cstdint>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace hft::net {

class UdpSocket {
public:
    UdpSocket() noexcept = default;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    ~UdpSocket() noexcept { close_(); }

    UdpSocket(UdpSocket&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    UdpSocket& operator=(UdpSocket&& o) noexcept {
        if (this != &o) { close_(); fd_ = o.fd_; o.fd_ = -1; }
        return *this;
    }

    /* 加入组播组。group/if_ip 为点分十进制（如 "239.1.2.3" / "127.0.0.1"）。
     * 返回 0 成功，-1 失败（errno 保留）。 */
    int join_multicast(const char* group, std::uint16_t port, const char* if_ip) noexcept {
        close_();
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) return -1;

        int reuse = 1;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);   // bind ANY 收组播
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close_();
            return -1;
        }

        ip_mreq mreq{};
        ::inet_pton(AF_INET, group, &mreq.imr_multiaddr);
        ::inet_pton(AF_INET, if_ip, &mreq.imr_interface);
        if (::setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            close_();
            return -1;
        }
        return 0;
    }

    /* 非阻塞收一个数据报。>0 = 字节数；0 = 暂无（EAGAIN）；-1 = 错误。 */
    int recv(std::uint8_t* buf, int cap) noexcept {
        const ssize_t n = ::recv(fd_, buf, static_cast<size_t>(cap), MSG_DONTWAIT);
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        return static_cast<int>(n);
    }

    /* 发送一个数据报到组播组（测试/重传响应模拟用）。 */
    static int send_to(const std::uint8_t* buf, int len,
                       const char* group, std::uint16_t port, const char* if_ip) noexcept {
        const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return -1;

        in_addr ifaddr{};
        ::inet_pton(AF_INET, if_ip, &ifaddr);
        ::setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        ::inet_pton(AF_INET, group, &addr.sin_addr);

        const ssize_t n = ::sendto(fd, buf, static_cast<size_t>(len), 0,
                                   reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        ::close(fd);
        return static_cast<int>(n);
    }

    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }

private:
    void close_() noexcept {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }
    int fd_ = -1;
};

} // namespace hft::net
