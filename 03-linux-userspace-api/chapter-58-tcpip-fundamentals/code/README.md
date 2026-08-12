# Ch58 demos — TCP/IP fundamentals (no listen)

```bash
gcc -Wall -Wextra -o inet_addr_demo inet_addr_demo.c
./inet_addr_demo 127.0.0.1 8080
```

| 文件 | 说明 |
|------|------|
| `inet_addr_demo.c` | `sockaddr_in` + htons + inet_pton/ntop |

## 代码示例

```c

/* TCP/IP 基础：inet_pton 地址转换 + sockaddr_in */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void) {
    /* inet_pton: 将点分十进制 IPv4 地址转为网络字节序 */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed\n"); exit(1);
    }
    printf("sockaddr_in: family=%d, port=%u, addr=0x%x\n",
           addr.sin_family, ntohs(addr.sin_port),
           ntohl(addr.sin_addr.s_addr));

    /* inet_ntop: 网络字节序转点分十进制 */
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ipstr, sizeof(ipstr));
    printf("address string: %s\n", ipstr);

    /* INADDR_ANY = 0.0.0.0 = 绑定所有接口 */
    struct sockaddr_in any_addr = { .sin_family = AF_INET, .sin_port = htons(9090) };
    any_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_ntop(AF_INET, &any_addr.sin_addr, ipstr, sizeof(ipstr));
    printf("INADDR_ANY: %s:%u\n", ipstr, ntohs(any_addr.sin_port));
    return 0;
}

```

---
