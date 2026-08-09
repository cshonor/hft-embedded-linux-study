# Ch58 demos — TCP/IP fundamentals (no listen)

```bash
gcc -Wall -Wextra -o inet_addr_demo inet_addr_demo.c
./inet_addr_demo 127.0.0.1 8080
```

| 文件 | 说明 |
|------|------|
| `inet_addr_demo.c` | `sockaddr_in` + htons + inet_pton/ntop |
