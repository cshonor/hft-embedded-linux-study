# Part A — mini shell

`fork`/`exec`、内建 `cd`/`pwd`/`exit`、管道 `|`、重定向 `<` `>`、行尾 `&` 后台。父进程忽略 SIGINT，SIGCHLD 收割僵尸。

```bash
make test
./myshell
```
