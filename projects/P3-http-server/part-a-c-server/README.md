# Part A — C echo / 最小 HTTP

loopback echo，以及 `GET /hello.txt` → 200、未知路径 → 404。`epoll` / 线程池 / 静态文件还没做，见 [../Part-A-c-server.md](../Part-A-c-server.md)。

```bash
make test
./echo_server
```
