# Ch59 demos — Internet domain sockets

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o tcp_iter_sv tcp_iter_sv.c
gcc -Wall -Wextra -o tcp_iter_cl tcp_iter_cl.c
gcc -Wall -Wextra -o udp_echo_sv udp_echo_sv.c
gcc -Wall -Wextra -o udp_echo_cl udp_echo_cl.c

# TCP (loopback)
./tcp_iter_sv 127.0.0.1 19059    # terminal A
./tcp_iter_cl 127.0.0.1 19059 hi # terminal B

# UDP
./udp_echo_sv 127.0.0.1 19060
./udp_echo_cl 127.0.0.1 19060 ping
```

全部用 `getaddrinfo`；服务端 `AI_PASSIVE` 风格（显式地址亦可）。

| 文件 | 说明 |
|------|------|
| `tcp_iter_*.c` | 迭代 TCP：一行请求/应答；忽略 SIGPIPE |
| `udp_echo_*.c` | UDP sendto/recvfrom 回显 |
