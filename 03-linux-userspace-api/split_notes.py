#!/usr/bin/env python3
"""
Split 03-linux-userspace-api notes.md into individual section files.
Each section gets a 150+ line file with standard skeleton:
  本节讲什么 -> 要点 -> HFT/嵌入式关联 -> 常见陷阱(3) -> 自测题(4) -> 衔接
"""

import os, re, glob

BASE = r"C:\Users\12392\Desktop\hft\03-linux-userspace-api"

# ========== Chinese -> English slug mapping ==========
CN_EN = {
    '概述':'overview','概念':'concepts','基本概念':'basic-concepts',
    '文件描述符':'file-descriptor','通用':'universal','模型':'model',
    '内存':'memory','进程':'process','线程':'thread',
    '信号':'signal','管道':'pipe','套接字':'socket','锁':'lock',
    '互斥':'mutex','条件变量':'condvar','环境变量':'environment',
    '地址空间':'address-space','错误处理':'error-handling',
    '系统调用':'syscall','权限':'permission','用户':'user',
    '组':'group','时间':'time','定时器':'timer','时钟':'clock',
    '目录':'directory','链接':'link','文件系统':'filesystem',
    '属性':'attributes','缓冲':'buffering','创建':'creation',
    '终止':'termination','等待':'wait','执行':'exec',
    '调度':'scheduling','优先级':'priority','资源':'resources',
    '守护':'daemon','安全':'security','能力':'capabilities',
    '共享库':'shared-library','消息队列':'message-queue',
    '信号量':'semaphore','共享内存':'shared-memory',
    '内存映射':'memory-mapping','虚拟内存':'virtual-memory',
    '文件锁':'file-locking','网络':'network','字节序':'byte-order',
    '服务器':'server','终端':'terminal','伪终端':'pseudoterminal',
    '生命周期':'lifecycle','编号':'numbering','分类':'classification',
    '掩码':'mask','未决':'pending','发送':'sending',
    '原子':'atomic','非阻塞':'nonblock','大文件':'large-file',
    '栈':'stack','堆':'heap','段':'segment','凭证':'credentials',
    '加密':'crypto','继承':'inheritance','封装':'encapsulation',
    '多态':'polymorphism','重载':'overloading','抽象':'abstraction',
    '访问控制':'access-control','数组':'array','指针':'pointer',
    '声明':'declaration','定义':'definition','类型转换':'type-conversion',
    '优先级规则':'precedence','联合':'union','位字段':'bitfield',
    '预处理器':'preprocessor','宏':'macro','函数':'function',
    '变量':'variable','常量':'constant','运算符':'operator',
    '控制流':'control-flow','分支':'branch','循环':'loop',
    '结构体':'struct','静态':'static','动态':'dynamic',
    '编译':'compile','载入':'loading','符号':'symbol',
    '初始化':'initialization','命名空间':'namespace',
    '存储类':'storage-class','对齐':'alignment','标志':'flag',
    '模式':'mode','状态':'state','事件':'event','通知':'notification',
    '回调':'callback','架构':'architecture','设计':'design',
    '优化':'optimization','性能':'performance','调试':'debugging',
    '测试':'testing','日志':'logging','监控':'monitoring',
    '配置':'configuration','规范':'standard','特征':'characteristics',
    '类型':'types','操作':'operations','原理':'principle',
    '机制':'mechanism','策略':'strategy','对比':'comparison',
    '差异':'difference','区别':'difference','限制':'limits',
    '工具':'tools','实践':'practice','陷阱':'pitfalls',
    '选型':'selection','持久性':'persistence','访问':'access',
    '范围':'scope','结构':'structure','管理':'management',
    '分配':'allocation','释放':'release','保护':'protection',
    '检测':'detection','更新':'update','读取':'read-op',
    '写入':'write-op','关闭':'close','复制':'dup',
    '阻塞':'blocking','容量':'capacity','局限':'limitations',
    '缺陷':'defects','限额':'quotas','运维':'ops',
    '算法':'algorithm','格式':'format','转换':'conversion',
    '解析':'parsing','序列化':'serialization','连接':'connection',
    '断开':'disconnect','重连':'reconnect','超时':'timeout',
    '重试':'retry','失败':'failure','成功':'success',
    '返回值':'return-value','参数':'parameter','处理':'handling',
    '注册':'register','注销':'deregister','查询':'query',
    '设置':'set','获取':'get','删除':'delete','清空':'clear',
    '插入':'insert','移除':'remove','查找':'search',
    '排序':'sort','过滤':'filter','映射':'map',
    '归约':'reduce','迭代':'iterate','遍历':'traverse',
    '生产':'produce','消费':'consume','同步':'sync',
    '异步':'async','并发':'concurrent','并行':'parallel',
}

# ========== Knowledge Base: pitfalls + questions per keyword ==========
# Format: (keyword, [3 pitfalls], [4 questions])
# Each pitfall: (title, code_block, explanation)
# Each question: (type, question_text, answer_text)

KB = []

def add(kw, pitfalls, questions):
    KB.append((kw, pitfalls, questions))

# --- File I/O ---
add('open', [
    ('O_CREAT 忘传 mode', '```c\n/* mode 参数丢失，权限不确定 */\nfd = open("f.txt", O_WRONLY | O_CREAT);\n/* 必须给 mode */\nfd = open("f.txt", O_WRONLY | O_CREAT, 0644);\n```', 'O_CREAT 时第三个参数 mode 是新文件权限，受 umask 影响。漏传会导致权限不确定（取决于栈上残留值）。'),
    ('返回值未检查', '```c\nint fd = open(path, O_RDONLY);\nread(fd, buf, n);  /* fd=-1 → EBADF */\n```', 'open 失败返回 -1。不检查直接用 fd 会导致后续操作全部失败。必须 `if ((fd = open(...)) < 0) { perror; exit; }`。'),
    ('O_TRUNC 意外截断', '```c\n/* 想追加写，却加了 O_TRUNC */\nfd = open("log.txt", O_WRONLY | O_TRUNC);  /* 清空了! */\nfd = open("log.txt", O_WRONLY | O_APPEND);  /* 正确 */\n```', 'O_TRUNC 在文件存在时截断为 0。追加写场景误加 O_TRUNC 会丢失数据。'),
], [
    ('选择', '`open("f.txt", O_CREAT)` 缺少什么参数？\nA. flags 不对\nB. mode 参数\nC. pathname\nD. 返回值', 'B。O_CREAT 时必须提供 mode（如 0644），受 umask 影响。漏传会使用栈上不确定的值。'),
    ('判断', '`open` 失败时返回 0。', '错误。open 失败返回 -1 并设置 errno。fd=0 是 stdin，open 正常时可能返回 0（如果 stdin 被关闭）。'),
    ('踩坑', '以下代码有什么问题？\n```c\nint fd = open("/etc/passwd", O_WRONLY|O_CREAT|O_TRUNC, 0644);\n```', '对 /etc/passwd 用 O_WRONLY|O_TRUNC 会截断系统密码文件！想读文件应用 O_RDONLY。这是非常危险的操作。'),
    ('应用', '写代码：以读写方式打开文件，不存在则创建（权限 0644），存在则追加。', '```c\nint fd = open("data.txt", O_RDWR|O_CREAT|O_APPEND, 0644);\nif (fd < 0) { perror("open"); exit(1); }\n```'),
])

add('read', [
    ('短读未处理', '```c\n/* 假设一次读完 */\nssize_t n = read(fd, buf, 1024);\n/* n 可能 < 1024 */\n```', 'read 返回值可能小于请求字节数（short read）。网络和管道尤其常见，必须循环读。'),
    ('EINTR 未处理', '```c\n/* 信号中断后 read 返回 -1, errno=EINTR */\n/* 不是错误，应重试 */\n```', '信号中断系统调用时返回 -1 且 errno=EINTR。应该重试，或用 SA_RESTART 自动重启。'),
    ('EOF 判断错误', '```c\n/* 用 feof 预判 EOF → 多读一次 */\nwhile (!feof(fp)) { fgets(...); }\n/* 正确：检查返回值 */\nwhile (fgets(buf, sz, fp) != NULL) { ... }\n```', 'read 返回 0 表示 EOF。不要用 feof 预判（feof 只在读取越过后才为真）。'),
], [
    ('选择', '`read()` 返回 0 表示什么？\nA. 出错\nB. EOF\nC. 没有数据\nD. 需要重试', 'B。返回 0 表示已到达文件末尾（EOF）。返回 -1 表示出错，返回 >0 表示读到的字节数。'),
    ('判断', '`read` 一定会读满请求的字节数。', '错误。read 可能返回少于请求的字节数（short read），尤其在管道、socket、终端上。必须循环读。'),
    ('踩坑', '以下代码有什么问题？\n```c\nchar buf[1024];\nread(fd, buf, sizeof(buf));\nbuf[strlen(buf)] = \'\\0\';\n```', '1) read 不保证以 null 结尾；2) 没检查返回值；3) strlen 在未终止的 buf 上越界。应检查返回值并手动加 `\\0`。'),
    ('应用', '写一个安全的 readall 函数，确保读满 n 字节或遇到 EOF。', '```c\nssize_t readall(int fd, void *buf, size_t n) {\n    size_t total = 0;\n    while (total < n) {\n        ssize_t r = read(fd, (char*)buf+total, n-total);\n        if (r < 0 && errno == EINTR) continue;\n        if (r < 0) return -1;\n        if (r == 0) break; /* EOF */\n        total += r;\n    }\n    return total;\n}\n```'),
])

add('write', [
    ('部分写未处理', '```c\n/* 假设一次写完 */\nwrite(fd, buf, 1024);\n/* 可能只写了 512 */\n```', 'write 返回值可能小于请求字节数（partial write）。必须循环写直到全部写出。'),
    ('write 成功 ≠ 落盘', '```c\nwrite(fd, data, size);  /* 数据在页缓存 */\nfsync(fd);  /* 强制落盘 */\n```', 'write 成功只表示数据到了内核页缓存。需要 fsync/fdatasync 确保持久化。'),
    ('SIGPIPE 导致进程退出', '```c\nwrite(pipe_fd, buf, n);  /* 对端关闭 → SIGPIPE */\nsignal(SIGPIPE, SIG_IGN);  /* 忽略 */\n```', '向已关闭的管道/socket 写数据触发 SIGPIPE，默认终止进程。应忽略 SIGPIPE。'),
], [
    ('选择', '`write()` 返回值小于请求字节数时应该？\nA. 报错退出\nB. 重试写剩余部分\nC. 忽略\nD. 关闭文件', 'B。这是部分写（partial write），是正常现象。应该循环写剩余部分直到全部写出。'),
    ('判断', '`write` 成功返回后数据已经写入磁盘。', '错误。write 成功只表示数据到了内核页缓存。断电仍会丢失。需要 fsync 确保落盘。'),
    ('踩坑', '以下代码有什么问题？\n```c\nint fd = open("log.txt", O_WRONLY);\nwrite(fd, "hello", 5);\nclose(fd);\n```', '1) 没检查 open 返回值；2) 没检查 write 返回值（可能部分写）；3) 没检查 close 返回值（NFS 上 close 可能报延迟错误）。'),
    ('应用', '写一个安全的 writeall 函数。', '```c\nssize_t writeall(int fd, const void *buf, size_t n) {\n    size_t total = 0;\n    while (total < n) {\n        ssize_t w = write(fd, (char*)buf+total, n-total);\n        if (w < 0 && errno == EINTR) continue;\n        if (w < 0) return -1;\n        total += w;\n    }\n    return total;\n}\n```'),
])

add('lseek', [
    ('管道上 lseek 失败', '```c\nlseek(pipe_fd, 0, SEEK_SET);  /* ESPIPE */\n```', '管道、socket、终端不支持 seek。lseek 返回 -1，errno=ESPIPE。'),
    ('空洞文件浪费磁盘', '```c\nlseek(fd, 1024*1024, SEEK_SET);\nwrite(fd, "x", 1);  /* 中间是空洞 */\n```', 'lseek 超过文件末尾再 write 会产生空洞。空洞在读时返回 0，但不一定占磁盘块。'),
    ('lseek 返回值未检查', '```c\nlseek(fd, offset, SEEK_SET);\nread(fd, buf, n);  /* 偏移可能没变 */\n```', 'lseek 返回新的文件偏移。不检查返回值可能导致后续读写位置错误。'),
], [
    ('选择', '`lseek(fd, 0, SEEK_CUR)` 的作用是？\nA. 跳到开头\nB. 跳到末尾\nC. 获取当前偏移\nD. 重置偏移', 'C。SEEK_CUR 从当前位置偏移 0 字节，相当于获取当前文件偏移量而不移动。'),
    ('判断', '所有文件类型都支持 lseek。', '错误。管道、socket、终端不支持 lseek，返回 -1 且 errno=ESPIPE。'),
    ('踩坑', '以下代码试图在管道上随机访问，会怎样？\n```c\nint fd = open("pipe", O_RDWR);\nlseek(fd, 100, SEEK_SET);\n```', '如果 fd 是管道，lseek 返回 -1 且 errno=ESPIPE。管道是流式数据，不支持随机访问。'),
    ('应用', '写代码获取文件大小（不用 stat）。', '```c\noff_t cur = lseek(fd, 0, SEEK_CUR);\noff_t size = lseek(fd, 0, SEEK_END);\nlseek(fd, cur, SEEK_SET);  /* 恢复 */\n```'),
])

add('close', [
    ('close 返回值未检查', '```c\nclose(fd);  /* NFS 上可能报延迟错误 */\n```', 'close 可能返回错误（尤其在 NFS 上）。不检查会丢失延迟的写错误。生产代码应检查。'),
    ('fd 泄漏', '```c\nint fd = open(...);\nif (error) return;  /* fd 泄漏! */\nclose(fd);\n```', '打开 fd 后，在所有错误路径上都必须 close。用 goto cleanup 模式或 RAII。'),
    ('double close', '```c\nclose(fd);\n/* ... */\nclose(fd);  /* 可能关闭了复用的 fd */\n```', 'close 后 fd 被回收。如果另一个 open 复用了同一编号，double close 会关错文件。close 后应置 -1。'),
], [
    ('选择', '`close(fd)` 后 fd 的值变为？\nA. 0\nB. -1\nC. 不变但不可用\nD. 自动设为 -1', 'C。close 不会修改 fd 变量的值，但该 fd 编号已被回收，可被后续 open 复用。建议手动置 -1。'),
    ('判断', '`close` 不可能失败。', '错误。close 可能失败，尤其在 NFS 上。延迟的写错误可能在 close 时才报告。'),
    ('踩坑', '以下代码有什么问题？\n```c\nint fd = open("f.txt", O_RDONLY);\nif (read(fd, buf, n) < 0)\n    return -1;\nclose(fd);\n```', 'read 失败时直接 return，没有 close(fd)，导致 fd 泄漏。应改为 goto cleanup 或在 return 前 close。'),
    ('应用', '写一个安全的文件读取函数，确保所有路径都关闭 fd。', '```c\nint read_file(const char *path, char *buf, size_t n) {\n    int fd = open(path, O_RDONLY);\n    if (fd < 0) return -1;\n    ssize_t r = read(fd, buf, n);\n    close(fd);\n    return (r < 0) ? -1 : r;\n}\n```'),
])

add('dup', [
    ('dup2 的 close-on-exec', '```c\ndup2(oldfd, newfd);  /* newfd 没有 CLOEXEC */\n```', 'dup2 复制的 fd 不继承 close-on-exec 标志。exec 前需要手动设置，或用 dup3(fd, newfd, O_CLOEXEC)。'),
    ('dup2 目标已打开', '```c\ndup2(oldfd, newfd);  /* 如果 newfd 已打开，先关闭 */\n```', 'dup2 如果 newfd 已打开，会先关闭再复制。如果不希望这样，应先检查。'),
    ('共享偏移的意外', '```c\nint fd2 = dup(fd1);\n/* fd1 和 fd2 共享文件偏移 */\nlseek(fd1, 0, SEEK_SET);  /* fd2 的偏移也变了! */\n```', 'dup 复制的 fd 共享同一个打开文件描述（包括偏移）。lseek 通过一个 fd 会影响另一个。'),
], [
    ('选择', '`dup2(oldfd, newfd)` 和 `dup(oldfd)` 的区别？\nA. 没有区别\nB. dup2 可以指定目标 fd 编号\nC. dup2 更快\nD. dup 只能复制到 0', 'B。dup 返回最小可用 fd，dup2 可以指定目标 fd 编号 newfd。'),
    ('判断', '`dup` 复制的 fd 有独立的文件偏移。', '错误。dup 复制的 fd 共享同一个打开文件描述，包括文件偏移和状态标志。'),
    ('踩坑', '以下代码在 exec 后会发生什么？\n```c\nint fd = open("f.txt", O_RDONLY);\ndup2(fd, STDIN_FILENO);\nexecvp("cat", argv);\n```', 'dup2 复制的 fd 默认没有 FD_CLOEXEC，所以 exec 后 stdin 仍然指向 f.txt。这正是 shell 重定向的原理。'),
    ('应用', '写代码实现 shell 的输出重定向 `cmd > file.txt`。', '```c\nint fd = open("file.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);\ndup2(fd, STDOUT_FILENO);\nclose(fd);\nexecvp(cmd, argv);\n```'),
])

add('fcntl', [
    ('fd 标志 vs 文件状态标志混淆', '```c\n/* 错误：F_SETFL 改不了访问模式 */\nfcntl(fd, F_SETFL, O_WRONLY);  /* 无效 */\n```', 'F_SETFL 只能改 O_APPEND/O_NONBLOCK 等，不能改 O_RDONLY/O_WRONLY/O_RDWR。访问模式在 open 时确定。'),
    ('F_GETFL 解析访问模式', '```c\nint flags = fcntl(fd, F_GETFL);\nint acc = flags & O_ACCMODE;  /* O_RDONLY/O_WRONLY/O_RDWR */\n```', '不能直接用 `flags & O_WRONLY` 判断，因为 O_RDONLY 通常是 0。必须用 O_ACCMODE 掩码。'),
    ('FD_CLOEXEC 竞态', '```c\nint fd = open(...);\nfcntl(fd, F_SETFD, FD_CLOEXEC);  /* open 和 fcntl 之间有间隙 */\n/* 多线程中 exec 可能在此间隙发生 */\n```', 'open + fcntl(F_SETFD) 之间存在竞态窗口。用 O_CLOEXEC 标志一步到位。'),
], [
    ('选择', '`fcntl(fd, F_SETFL, flags)` 可以改变以下哪个标志？\nA. O_RDONLY\nB. O_WRONLY\nC. O_NONBLOCK\nD. O_CREAT', 'C。F_SETFL 只能改 O_APPEND、O_NONBLOCK、O_ASYNC 等状态标志，不能改访问模式（O_RDONLY 等）。'),
    ('判断', '`FD_CLOEXEC` 是文件状态标志，存在打开文件描述中。', '错误。FD_CLOEXEC 是 fd 标志，存在进程 fd 表中，每个 fd 私有。文件状态标志（如 O_APPEND）才在打开文件描述中。'),
    ('踩坑', '以下代码试图将文件改为非阻塞，有什么问题？\n```c\nfcntl(fd, F_SETFL, O_NONBLOCK);\n```', '问题：F_SETFL 会覆盖所有状态标志。应该先 F_GETFL 取回当前标志，再 OR 上 O_NONBLOCK：\n`fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);`'),
    ('应用', '写代码设置 fd 为非阻塞模式。', '```c\nint flags = fcntl(fd, F_GETFL);\nif (flags < 0) return -1;\nreturn fcntl(fd, F_SETFL, flags | O_NONBLOCK);\n```'),
])

# --- Processes ---
add('fork', [
    ('fork 返回值未区分', '```c\npid_t pid = fork();\n/* 父子都执行同一代码 */\n```', 'fork 返回两次：父进程返回子 PID，子进程返回 0，失败返回 -1。必须区分三个分支。'),
    ('fd 继承导致泄漏', '```c\nint fd = open(...);\nfork();\n/* 父子都有 fd，都不关 → 泄漏 */\n```', 'fork 后子进程继承父的 fd 表。父子都需要 close 各自的 fd。'),
    ('缓冲区被复制', '```c\nprintf("hello");  /* 在缓冲区，未 flush */\nfork();  /* 子进程也会输出 hello */\n```', 'fork 前如果 stdio 缓冲区有数据，子进程会复制并重复输出。fork 前应 fflush 或用 write。'),
], [
    ('选择', '`fork()` 在子进程中返回什么？\nA. 子进程 PID\nB. 0\nC. -1\nD. 父进程 PID', 'B。子进程中 fork 返回 0，父进程中返回子进程 PID，失败返回 -1。'),
    ('判断', '`fork` 后子进程和父进程的执行顺序是确定的。', '错误。fork 后父子进程的执行顺序不确定，取决于调度器。不能用 sleep 来保证顺序。'),
    ('踩坑', '以下代码有什么问题？\n```c\nprintf("before fork\\n");\nfork();\nprintf("after fork\\n");\n```', '如果 stdout 是行缓冲（终端），"before fork" 会立即输出。但如果是全缓冲（管道/文件），缓冲区被复制，子进程也会输出 "before fork"。fork 前应 fflush(stdout)。'),
    ('应用', '写代码：fork 一个子进程，父进程打印子 PID，子进程打印自己的 PID。', '```c\npid_t pid = fork();\nif (pid < 0) { perror("fork"); exit(1); }\nif (pid == 0) {\n    printf("child: %d\\n", getpid());\n} else {\n    printf("parent: child=%d\\n", pid);\n}\n```'),
])

add('exec', [
    ('fd 泄漏到 exec', '```c\nint fd = open(...);\nexecvp(prog, argv);  /* fd 仍然打开 */\n```', '默认情况下 exec 后 fd 仍然打开。设 FD_CLOEXEC 或用 O_CLOEXEC 防止泄漏。'),
    ('exec 后代码继续执行', '```c\nexecvp(prog, argv);\nprintf("这里会执行吗?");  /* 会! exec 失败时 */\n```', 'exec 成功时不返回，当前进程映像被替换。只有 exec 失败时才返回 -1。必须检查返回值。'),
    ('PATH 搜索差异', '```c\nexeclp("ls", "ls", NULL);   /* 搜索 PATH */\nexecl("/bin/ls", "ls", NULL);  /* 不搜索 PATH */\n```', 'execlp/execvp 搜索 PATH，execl/execv 不搜索。用错可能找不到程序。'),
], [
    ('选择', '`exec` 成功后会发生什么？\nA. 创建新进程\nB. 替换当前进程映像\nC. 返回旧进程\nD. 什么也不发生', 'B。exec 用新程序替换当前进程映像。PID 不变，代码段/数据段/堆栈被替换。成功时不返回。'),
    ('判断', '`exec` 失败时返回 -1，原程序继续执行。', '正确。exec 失败时返回 -1，当前进程映像不变，代码继续执行。成功时不返回。'),
    ('踩坑', '以下代码有什么问题？\n```c\nexecvp("ls", args);\nperror("exec failed");\n```', '没有检查 execvp 返回值。应该：\n`if (execvp("ls", args) < 0) { perror("exec"); exit(1); }`\nperror 本身没问题，但应该配合 exit 确保失败后退出。'),
    ('应用', '写代码：fork + exec 执行 `ls -l`，父进程等待子进程结束。', '```c\npid_t pid = fork();\nif (pid == 0) {\n    execlp("ls", "ls", "-l", NULL);\n    perror("exec"); exit(1);\n}\nwait(NULL);\n```'),
])

add('wait', [
    ('僵尸进程', '```c\nfork();\n/* 父进程不 wait → 子进程变僵尸 */\n```', '子进程退出后如果父进程不 wait，子进程变成僵尸（Z 状态），占用 PID 和少量内存。'),
    ('wait 只等第一个', '```c\nwait(&status);  /* 只等任意一个子进程 */\n```', 'wait 等待任意一个子进程。如果有多个子进程，需要循环 wait 或用 waitpid 指定 PID。'),
    ('status 宏使用错误', '```c\nwait(&status);\nif (status == 0) ...  /* 错误 */\nif (WIFEXITED(status)) ...  /* 正确 */\n```', '不能直接用 status 值判断。必须用 WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG 宏。'),
], [
    ('选择', '`wait(&status)` 中 status 的值 0 表示什么？\nA. 子进程退出码 0\nB. 不能直接判断\nC. 子进程被信号杀死\nD. 等待失败', 'B。不能直接用 status 值判断。必须用 WIFEXITED(status) 等宏解析。status=9 可能表示被 SIGKILL 杀死，也可能不是。'),
    ('判断', '`waitpid(pid, &status, WNOHANG)` 会阻塞等待子进程退出。', '错误。WNOHANG 表示非阻塞，如果子进程还没退出立即返回 0。不带 WNOHANG 才会阻塞。'),
    ('踩坑', '以下代码有什么问题？\n```c\npid_t pid = fork();\nif (pid == 0) exit(42);\nwait(&status);\nprintf("exit code: %d\\n", status);\n```', 'status 不能直接用。应该：\n`WEXITSTATUS(status)` 提取退出码。\n`printf("exit code: %d\\n", WEXITSTATUS(status));`'),
    ('应用', '写代码等待指定子进程，获取退出码或终止信号。', '```c\nint status;\nwaitpid(pid, &status, 0);\nif (WIFEXITED(status))\n    printf("exit: %d\\n", WEXITSTATUS(status));\nelse if (WIFSIGNALED(status))\n    printf("signal: %d\\n", WTERMSIG(status));\n```'),
])

# --- Memory ---
add('malloc', [
    ('返回值未检查', '```c\nint *p = malloc(sizeof(int) * 1000);\np[0] = 42;  /* malloc 失败 → NULL → 段错误 */\n```', 'malloc 失败返回 NULL。不检查直接用会导致段错误。Linux 上可能 OOM killer 先杀进程。'),
    ('use-after-free', '```c\nfree(p);\np[0] = 42;  /* UB! */\n```', 'free 后访问已释放内存是未定义行为。free 后应立即置 NULL：`free(p); p = NULL;`。'),
    ('realloc 指针失效', '```c\nint *p = malloc(100);\nint *q = realloc(p, 200);\n/* p 可能已失效，不能再用 */\n```', 'realloc 可能返回新地址并释放旧地址。realloc 后不能再用旧指针。应 `p = realloc(p, size);`。'),
], [
    ('选择', '`malloc(0)` 的返回值是？\nA. NULL\nB. 非 NULL 或 NULL（实现定义）\nC. 一定非 NULL\nD. 未定义行为', 'B。malloc(0) 的行为是实现定义的：可能返回 NULL，也可能返回一个非 NULL 但不可解引用的指针。'),
    ('判断', '`free(NULL)` 是未定义行为。', '错误。free(NULL) 是安全的，什么都不做。C 标准明确允许。'),
    ('踩坑', '以下代码有什么问题？\n```c\nchar *p = malloc(10);\nstrcpy(p, "Hello, World!");\n```', '缓冲区溢出。"Hello, World!" 是 13 字节（含 \\0），但只分配了 10 字节。应该 malloc(strlen(s)+1)。'),
    ('应用', '写一个安全的字符串复制函数。', '```c\nchar *safe_strdup(const char *s) {\n    char *p = malloc(strlen(s) + 1);\n    if (p) strcpy(p, s);\n    return p;\n}\n```'),
])

add('mmap', [
    ('MAP_FAILED vs NULL', '```c\nvoid *p = mmap(NULL, size, PROT_READ|PROT_WRITE,\n              MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);\nif (p == NULL) ...  /* 错误! */\nif (p == MAP_FAILED) ...  /* 正确 */\n```', 'mmap 失败返回 MAP_FAILED ((void*)-1)，不是 NULL。检查方式与 malloc 不同。'),
    ('munmap 忘记调用', '```c\nvoid *p = mmap(NULL, 4096, ...);\n/* 使用 p ... */\n/* 没有 munmap → 内存泄漏 */\n```', 'mmap 分配的内存不会自动释放。必须 munmap(p, size) 显式释放。'),
    ('信号处理中的 SIGSEGV', '```c\nmmap(NULL, size, PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);\n/* 写只读映射 → SIGSEGV */\n```', '对 PROT_READ 的映射写入会触发 SIGSEGV。确保保护属性和访问操作匹配。'),
], [
    ('选择', '`mmap` 失败时返回什么？\nA. NULL\nB. -1\nC. MAP_FAILED\nD. 0', 'C。mmap 失败返回 MAP_FAILED（定义为 (void *)-1），不是 NULL。这与 malloc 不同。'),
    ('判断', '`mmap` 映射的内存会在进程退出时自动释放。', '正确。进程退出时所有映射自动解除。但在长时间运行的程序中仍需手动 munmap。'),
    ('踩坑', '以下代码有什么问题？\n```c\nvoid *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE,\n               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);\nmemset(p, 0, 8192);\n```', '写入了 8192 字节但只映射了 4096 字节。超出映射区域会触发 SIGSEGV。应确保操作不超过映射大小。'),
    ('应用', '写代码用 mmap 分配匿名内存并初始化为 0。', '```c\nvoid *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE,\n               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);\nif (p == MAP_FAILED) { perror("mmap"); exit(1); }\nmemset(p, 0, 4096);\n/* ... 使用 ... */\nmunmap(p, 4096);\n```'),
])

# --- Signals ---
add('signal', [
    ('signal() 不可移植', '```c\nsignal(SIGINT, handler);  /* 行为不一致 */\n```', 'signal() 在不同系统上行为不一致（是否自动恢复默认、handler 期间是否阻塞自身）。生产代码用 sigaction()。'),
    ('handler 中调用不安全函数', '```c\nvoid handler(int sig) {\n    printf("got signal\\n");  /* printf 不安全! */\n}\n```', 'printf/malloc 等不是异步信号安全函数。handler 中只能调用异步信号安全函数（如 write）。'),
    ('信号竞态', '```c\n/* 检查标志后处理，但信号可能在此间到达 */\nif (flag) { ... }  /* TOCTOU */\n```', '信号可能在检查和处理之间到达。需要用 sigprocmask 阻塞信号或用 volatile sig_atomic_t。'),
], [
    ('选择', '`SIGKILL` 和 `SIGSTOP` 的共同特点是？\nA. 可以被捕获\nB. 可以被忽略\nC. 可以被阻塞\nD. 都不能被捕获/忽略/阻塞', 'D。SIGKILL 和 SIGSTOP 不能被捕获、忽略或阻塞。这是设计上的保证。'),
    ('判断', '可以在信号处理函数中安全地调用 `printf`。', '错误。printf 不是异步信号安全函数。它使用 stdio 缓冲区和 malloc，在信号处理函数中调用可能导致死锁或数据损坏。用 write 代替。'),
    ('踩坑', '以下代码有什么问题？\n```c\nvolatile int flag = 0;\nvoid handler(int sig) { flag = 1; }\nint main() {\n    signal(SIGINT, handler);\n    while (!flag) { /* busy wait */ }\n    printf("done\\n");\n}\n```', '1) signal() 不可移植，应用 sigaction；2) busy wait 浪费 CPU，应用 pause() 或 sigsuspend()；3) printf 在信号后的主循环中调用是安全的，但如果信号在 printf 执行中到来则不安全。'),
    ('应用', '写代码用 sigaction 捕获 SIGINT 并设置标志。', '```c\nvolatile sig_atomic_t flag = 0;\nvoid handler(int sig) { flag = 1; }\nstruct sigaction sa = {0};\nsa.sa_handler = handler;\nsigemptyset(&sa.sa_mask);\nsa.sa_flags = SA_RESTART;\nsigaction(SIGINT, &sa, NULL);\n```'),
])

add('kill', [
    ('kill 不等于杀死', '```c\nkill(pid, SIGTERM);  /* 发送 SIGTERM，不是杀死 */\n```', 'kill 是"发送信号"，不是"杀死进程"。SIGTERM 可以被捕获和处理。只有 SIGKILL 才强制杀死。'),
    ('权限不足', '```c\nkill(other_pid, SIGTERM);  /* EPERM if not owner */\n```', '只能给自己同 UID 的进程发信号（root 除外）。权限不足返回 -1，errno=EPERM。'),
    ('pid 参数含义', '```c\nkill(0, sig);   /* 发给同进程组 */\nkill(-1, sig);  /* 发给所有有权限的 */\nkill(-pgid, sig); /* 发给进程组 */\n```', 'pid 的正/负/零含义不同。用错可能误发信号给不该收的进程。'),
], [
    ('选择', '`kill(pid, 0)` 的作用是？\nA. 杀死进程\nB. 发送信号 0\nC. 检查进程是否存在/是否有权限\nD. 无操作', 'C。信号 0 不实际发送信号，但会进行权限检查。返回 0 表示进程存在且有权限，-1 表示不存在或无权限。'),
    ('判断', '`kill(getpid(), SIGKILL)` 等价于 `exit()`。', '错误。SIGKILL 不会执行 atexit 处理器、不会 flush stdio 缓冲、不会做资源清理。exit() 会执行清理。'),
    ('踩坑', '以下代码有什么风险？\n```c\npid_t pid = fork();\nif (pid == 0) {\n    /* child */\n    sleep(100);\n}\nkill(pid, SIGTERM);  /* 父进程发信号 */\n```', '如果 fork 失败（返回 -1），pid=-1，kill(-1, SIGTERM) 会给所有有权限的进程发信号！必须先检查 fork 返回值。'),
    ('应用', '写代码：给指定进程发 SIGTERM，等 1 秒后如果还没退出则发 SIGKILL。', '```c\nkill(pid, SIGTERM);\nsleep(1);\nif (waitpid(pid, NULL, WNOHANG) == 0) {\n    kill(pid, SIGKILL);\n    waitpid(pid, NULL, 0);\n}\n```'),
])

# --- Threads ---
add('pthread', [
    ('返回值 vs errno', '```c\nif (pthread_create(&t, NULL, func, arg) < 0)  /* 错误! */\n    perror("pthread_create");\n/* 正确：返回错误码，不是 -1 */\nif ((err = pthread_create(&t, NULL, func, arg)) != 0)\n    strerror(err);\n```', 'Pthreads 函数返回错误码而不是设置 errno。不能用 perror，要用 strerror(err)。'),
    ('忘记 join 或 detach', '```c\npthread_create(&t, NULL, func, arg);\n/* 不 join 也不 detach → 资源泄漏 */\n```', '线程结束后如果不 join 或 detach，资源不会回收。必须 join 或 detach。'),
    ('mutex 忘记 unlock', '```c\npthread_mutex_lock(&m);\nif (error) return;  /* 没 unlock! */\npthread_mutex_unlock(&m);\n```', 'mutex lock 后在所有路径都必须 unlock。用 goto cleanup 或 pthread_mutex_trylock + cleanup。'),
], [
    ('选择', '`pthread_create` 失败时返回什么？\nA. -1\nB. 0\nC. 错误码（正整数）\nD. NULL', 'C。Pthreads 函数返回错误码（如 EAGAIN）而不是 -1，也不设置 errno。用 strerror(err) 获取描述。'),
    ('判断', '主线程退出时其他线程会继续运行。', '错误。主线程调用 exit() 或从 main() 返回时，所有线程都会被终止。只有 pthread_exit 才允许其他线程继续。'),
    ('踩坑', '以下代码有什么问题？\n```c\nvoid *thread_func(void *arg) {\n    int x = *(int*)arg;\n    return NULL;\n}\nint val = 42;\npthread_create(&t, NULL, thread_func, &val);\n```', '如果 thread_func 在 val 被修改后才执行，会读到错误的值。应确保 arg 的生命周期覆盖线程执行期，或动态分配。'),
    ('应用', '写代码创建线程、等待线程结束并获取返回值。', '```c\nvoid *func(void *arg) { return (void*)42; }\npthread_t t;\npthread_create(&t, NULL, func, NULL);\nvoid *retval;\npthread_join(t, &retval);\nprintf("result: %ld\\n", (long)retval);\n```'),
])

add('mutex', [
    ('死锁', '```c\npthread_mutex_lock(&a);\npthread_mutex_lock(&b);  /* 如果另一线程先 lock b → 死锁 */\n```', '多个 mutex 的加锁顺序不一致会导致死锁。所有线程必须按相同顺序加锁。'),
    ('忘记 unlock', '```c\npthread_mutex_lock(&m);\nif (error) return;  /* 没 unlock */\npthread_mutex_unlock(&m);\n```', '所有代码路径都必须 unlock。用 goto cleanup 或 C++ RAII。'),
    ('优先级反转', '```c\n/* 低优先级线程持有 mutex */\n/* 高优先级线程等待 mutex → 被低优先级阻塞 */\n```', '低优先级线程持有锁时高优先级线程被阻塞。用 PTHREAD_PRIO_INHERIT 或避免在关键路径上加锁。'),
], [
    ('选择', '`pthread_mutex_trylock` 返回 EBUSY 表示什么？\nA. 系统繁忙\nB. mutex 已被其他线程持有\nC. mutex 不存在\nD. 参数无效', 'B。mutex 已被其他线程持有，trylock 不会阻塞而是立即返回 EBUSY。'),
    ('判断', '同一个线程可以对同一个 mutex 加锁两次。', '错误（默认情况下）。普通 mutex 不可重入，同线程第二次 lock 会死锁。需要可重入 mutex 用 PTHREAD_MUTEX_RECURSIVE。'),
    ('踩坑', '以下代码有什么问题？\n```c\npthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;\npthread_mutex_lock(&m);\n/* 处理数据 */\nif (error)\n    exit(1);\npthread_mutex_unlock(&m);\n```', 'error 时直接 exit(1)，没有 unlock mutex。虽然 exit 会终止进程（mutex 随之销毁），但如果改为 return 就会死锁。应养成所有路径都 unlock 的习惯。'),
    ('应用', '写代码用 mutex 保护一个计数器。', '```c\npthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;\nint counter = 0;\nvoid increment(void) {\n    pthread_mutex_lock(&m);\n    counter++;\n    pthread_mutex_unlock(&m);\n}\n```'),
])

add('condvar', [
    ('虚假唤醒', '```c\n/* 错误：用 if */\npthread_mutex_lock(&m);\nif (ready)  /* 可能虚假唤醒 */\n    pthread_cond_wait(&cv, &m);\n/* 正确：用 while */\nwhile (!ready)\n    pthread_cond_wait(&cv, &m);\n```', 'pthread_cond_wait 可能被虚假唤醒（spurious wakeup）。必须用 while 循环检查条件，不能用 if。'),
    ('忘记加锁', '```c\npthread_cond_signal(&cv);  /* 没有持有 mutex */\n```', 'signal/broadcast 通常应在持有 mutex 时调用，否则可能在 wait 之前 signal 导致丢失唤醒。'),
    ('条件变量未初始化', '```c\npthread_cond_t cv;  /* 未初始化 */\npthread_cond_wait(&cv, &m);  /* UB */\n```', '条件变量必须初始化：用 PTHREAD_COND_INITIALIZER 或 pthread_cond_init。'),
], [
    ('选择', '`pthread_cond_wait` 会做什么？\nA. 只等待\nB. 原子地解锁 mutex 并等待，被唤醒时重新加锁\nC. 加锁并等待\nD. 什么都不做', 'B。cond_wait 原子地释放 mutex 并进入等待。被唤醒时自动重新获取 mutex 后返回。'),
    ('判断', '`pthread_cond_wait` 被唤醒后条件一定为真。', '错误。可能被虚假唤醒。必须用 while 循环重新检查条件：\n`while (!condition) pthread_cond_wait(&cv, &m);`'),
    ('踩坑', '以下代码有什么问题？\n```c\npthread_mutex_lock(&m);\nif (count == 0)\n    pthread_cond_wait(&cv, &m);\ncount--;\npthread_mutex_unlock(&m);\n```', '用了 if 而不是 while。虚假唤醒时 count 可能仍然是 0，count-- 会下溢。改为 `while (count == 0)`。'),
    ('应用', '写代码：生产者-消费者模型，用 mutex + condvar。', '```c\npthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;\npthread_cond_t cv = PTHREAD_COND_INITIALIZER;\nint queue_size = 0;\nvoid consumer(void) {\n    pthread_mutex_lock(&m);\n    while (queue_size == 0)\n        pthread_cond_wait(&cv, &m);\n    queue_size--;\n    pthread_mutex_unlock(&m);\n}\nvoid producer(void) {\n    pthread_mutex_lock(&m);\n    queue_size++;\n    pthread_cond_signal(&cv);\n    pthread_mutex_unlock(&m);\n}\n```'),
])

# --- IPC ---
add('pipe', [
    ('fd 方向搞反', '```c\nint fds[2];\npipe(fds);\nwrite(fds[0], buf, n);  /* 错! fds[0] 是读端 */\nwrite(fds[1], buf, n);  /* 正确 */\n```', 'pipe 返回的 fd[0] 是读端，fd[1] 是写端。搞反会返回 EBADF。'),
    ('SIGPIPE', '```c\n/* 读端已关闭 */\nclose(fds[0]);\nwrite(fds[1], buf, n);  /* SIGPIPE → 进程被杀 */\n```', '写已关闭的管道会触发 SIGPIPE。应忽略 SIGPIPE 或用 send 的 MSG_NOSIGNAL。'),
    ('管道容量有限', '```c\n/* 写满管道后 write 阻塞 */\nwrite(fds[1], big_buf, 65536);  /* PIPE_BUF 通常 4096-65536 */\n```', '管道容量有限（通常 64KB）。写满后 write 阻塞。PIPE_BUF 以内的写是原子的。'),
], [
    ('选择', '`pipe(fds)` 中 `fds[0]` 和 `fds[1]` 分别是？\nA. 都是读端\nB. fds[0] 写端, fds[1] 读端\nC. fds[0] 读端, fds[1] 写端\nD. 都是写端', 'C。fds[0] 是读端，fds[1] 是写端。记忆法：0 像 O_RDONLY，1 像 O_WRONLY。'),
    ('判断', '管道的写端关闭后，读端 read 会返回 0。', '正确。所有写端关闭后，read 返回 0（EOF）。这是管道通信结束的标志。'),
    ('踩坑', '以下代码有什么问题？\n```c\nint fds[2];\npipe(fds);\nif (fork() == 0) {\n    close(fds[0]);\n    write(fds[1], "hello", 5);\n}\n```', '子进程关闭了读端，但父进程没有关闭写端。父进程的写端不关，子进程的 read 永远不会收到 EOF。需要在父子进程各自关闭不用的端。'),
    ('应用', '写代码：父子进程通过管道通信，子进程发消息给父进程。', '```c\nint fds[2];\npipe(fds);\nif (fork() == 0) {\n    close(fds[0]);\n    write(fds[1], "hello", 5);\n    close(fds[1]);\n} else {\n    close(fds[1]);\n    char buf[16];\n    read(fds[0], buf, 16);\n    printf("got: %s\\n", buf);\n}\n```'),
])

# --- Sockets ---
add('socket', [
    ('SIGPIPE', '```c\nsend(fd, buf, n, 0);  /* 对端关闭 → SIGPIPE */\nsend(fd, buf, n, MSG_NOSIGNAL);  /* 安全 */\n```', '向已关闭连接的 socket 写数据触发 SIGPIPE。用 MSG_NOSIGNAL 或忽略 SIGPIPE。'),
    ('TIME_WAIT', '```c\nbind(fd, (struct sockaddr*)&addr, len);\n/* EADDRINUSE if port in TIME_WAIT */\n```', '服务端重启时端口可能还在 TIME_WAIT。用 SO_REUSEADDR 允许绑定。'),
    ('accept 返回值未检查', '```c\nint cfd = accept(lfd, NULL, NULL);\n/* cfd 可能 -1 (EMFILE/ENFILE) */\n```', 'accept 可能因 fd 耗尽返回 -1。不检查直接用会导致后续操作失败。'),
], [
    ('选择', '`listen()` 的第二个参数 backlog 表示什么？\nA. 最大连接数\nB. 等待 accept 的队列长度\nC. 缓冲区大小\nD. 超时秒数', 'B。backlog 是已完成连接但尚未 accept 的队列长度。超过后新连接可能被拒绝。'),
    ('判断', '`bind` 可以绑定任意端口号。', '错误。1-1023 是特权端口，需要 root 权限。已占用的端口会返回 EADDRINUSE。'),
    ('踩坑', '以下代码有什么问题？\n```c\nint sfd = socket(AF_INET, SOCK_STREAM, 0);\nconnect(sfd, (struct sockaddr*)&addr, sizeof(addr));\nsend(sfd, "data", 4, 0);\n```', '没有检查 connect 返回值。connect 可能失败（连接被拒绝、超时等），不检查直接 send 会导致错误。'),
    ('应用', '写代码创建 TCP 服务端 socket 并监听。', '```c\nint sfd = socket(AF_INET, SOCK_STREAM, 0);\nint opt = 1;\nsetsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));\nstruct sockaddr_in addr = {0};\naddr.sin_family = AF_INET;\naddr.sin_port = htons(8080);\naddr.sin_addr.s_addr = INADDR_ANY;\nbind(sfd, (struct sockaddr*)&addr, sizeof(addr));\nlisten(sfd, 128);\n```'),
])

add('epoll', [
    ('ET 模式未读完', '```c\n/* Edge Triggered: 只通知一次 */\nepoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);  /* ev.events = EPOLLIN|EPOLLET */\n/* 必须循环读到 EAGAIN */\n```', 'ET 模式下事件只通知一次。如果没读完，下次不会再通知。必须循环读到 EAGAIN。'),
    ('EPOLL_CTL_ADD 重复添加', '```c\nepoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);\n/* 下次又 ADD 同一 fd → EEXIST */\n```', '已添加的 fd 再次 ADD 会返回 EEXIST。应该用 EPOLL_CTL_MOD 修改。'),
    ('fd 关闭后未从 epoll 移除', '```c\nclose(fd);  /* epoll 仍引用 → 可能通知已关闭的 fd */\n```', 'close fd 后 epoll 会自动移除，但如果 fd 被复用可能收到旧事件。建议先 epoll_ctl(DEL) 再 close。'),
], [
    ('选择', 'epoll 的 LT（水平触发）和 ET（边沿触发）的区别？\nA. 没有区别\nB. LT 只要数据在就一直通知，ET 只在状态变化时通知一次\nC. ET 比 LT 快\nD. LT 比 ET 快', 'B。LT 只要 fd 上有可读/可写数据就一直通知。ET 只在状态变化（如新数据到达）时通知一次，必须一次读完。'),
    ('判断', '`epoll_wait` 返回的事件中，fd 一定有数据可读。', '不一定。可能是错误事件（EPOLLERR）、挂起事件（EPOLLHUP）等。需要检查 events 字段。'),
    ('踩坑', '以下 ET 模式代码有什么问题？\n```c\n/* EPOLLET 模式 */\nchar buf[1024];\nint n = read(fd, buf, sizeof(buf));\nif (n > 0) process(buf, n);\n```', 'ET 模式下只读了一次。如果还有数据没读完，不会再收到通知。必须循环读直到返回 EAGAIN：\n`while ((n = read(fd, buf, sizeof(buf))) > 0) process(buf, n);`'),
    ('应用', '写代码：创建 epoll，添加一个监听 socket。', '```c\nint epfd = epoll_create1(0);\nstruct epoll_event ev;\nev.events = EPOLLIN;\nev.data.fd = listen_fd;\nepoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);\n```'),
])

# --- Generic fallback ---
GENERIC_PITFALLS = [
    ('返回值未检查', '```c\n/* 调用后不检查返回值直接继续 */\nresult = some_call(...);\nuse(result);  /* 如果失败? */\n```', '系统调用和库函数的返回值必须检查。失败时通常返回 -1 或 NULL 并设置 errno。不检查会导致后续操作在错误状态下继续。'),
    ('资源泄漏', '```c\n/* 打开资源后在错误路径未释放 */\nfd = open(...);\nif (error) return;  /* fd 泄漏 */\n```', '所有分配的资源（fd、内存、锁）在所有代码路径上都必须释放。用 goto cleanup 模式确保清理。'),
    ('可移植性假设', '```c\n/* 假设特定平台行为 */\n/* 如假设 int 是 4 字节、指针是 8 字节 */\n```', '不要硬编码平台相关假设。用 sizeof、stdint.h 类型（uint32_t 等）和 POSIX 宏确保可移植性。'),
]

GENERIC_QUESTIONS = [
    ('选择', '系统调用失败时通常如何报告错误？\nA. 打印错误信息\nB. 返回 -1 并设置 errno\nC. 调用 exit\nD. 抛出异常', 'B。系统调用失败时返回 -1 并设置 errno。用 perror 或 strerror(errno) 获取描述。Pthreads 例外，返回错误码。'),
    ('判断', 'errno 在函数成功时会被清零。', '错误。errno 只在失败时被设置，成功时不会清零。不能通过 errno 判断是否出错，必须先检查返回值。'),
    ('踩坑', '以下代码有什么问题？\n```c\nif (some_call() < 0) {\n    printf("error: %s\\n", strerror(errno));\n    /* 但中间可能调用了其他函数，errno 被覆盖 */\n}\n```', 'errno 可能在 strerror 调用前被其他函数覆盖。应该先保存 errno：`int e = errno;` 然后用 strerror(e)。或在 some_call 失败后立即处理。'),
    ('应用', '写一个错误处理模板：调用系统调用，失败时打印错误并退出。', '```c\nif (some_syscall(args) < 0) {\n    perror("some_syscall");\n    exit(EXIT_FAILURE);\n}\n/* 成功继续 */\n```'),
]

# ========== Helper Functions ==========

def slugify(title, section_num):
    """Convert a section title to an ASCII slug."""
    # Extract API names (English words in backticks or parentheses)
    apis = re.findall(r'`([a-zA-Z_][a-zA-Z0-9_]*)`', title)
    if not apis:
        apis = re.findall(r'\b([a-zA-Z_][a-zA-Z0-9_]*)\b', title)
    # Filter out common non-API words
    stop = {'the','a','an','of','and','or','not','vs','in','on','at','to','for','is','are','with','Ch','Linux','UNIX','POSIX','SUS'}
    apis = [a.lower() for a in apis if a not in stop and len(a) > 1]
    if apis:
        slug = '-'.join(apis[:3])
    else:
        # Try Chinese to English mapping
        slug_parts = []
        for cn, en in sorted(CN_EN.items(), key=lambda x: -len(x[0])):
            if cn in title:
                slug_parts.append(en)
                title = title.replace(cn, '')
        if slug_parts:
            slug = '-'.join(slug_parts[:3])
        else:
            slug = f'section-{section_num.replace(".", "-")}'
    # Clean up
    slug = re.sub(r'[^a-z0-9-]', '', slug)
    slug = re.sub(r'-+', '-', slug).strip('-')
    return slug if slug else f'section-{section_num.replace(".", "-")}'

def find_kb_match(title, content):
    """Find the best knowledge base entry for a section."""
    text = (title + ' ' + content).lower()
    best_match = None
    best_score = 0
    for kw, pitfalls, questions in KB:
        score = 0
        # Check if keyword appears in title or content
        if kw in title.lower():
            score += 10
        if kw in text:
            score += 5
        # Check for API name patterns
        if f'`{kw}' in content or f' {kw}(' in content or f' {kw} ' in content:
            score += 3
        if score > best_score:
            best_score = score
            best_match = (pitfalls, questions)
    return best_match

# ========== Parser ==========

def parse_notes_md(notes_path):
    """Parse notes.md into header, sections, and chapter-level content."""
    with open(notes_path, 'r', encoding='utf-8') as f:
        content = f.read()

    lines = content.split('\n')

    # Extract chapter number from directory name
    dir_name = os.path.basename(os.path.dirname(notes_path))
    ch_match = re.match(r'chapter-(\d+)', dir_name)
    ch_num = ch_match.group(1) if ch_match else '00'

    # Extract header (everything before first ## section or 章节目标)
    header_lines = []
    first_section_idx = None
    for i, line in enumerate(lines):
        if re.match(r'^## ', line):
            first_section_idx = i
            break
        header_lines.append(line)
    header = '\n'.join(header_lines)

    # Extract chapter title from first line
    title_match = re.match(r'^#\s+(.+)$', header_lines[0]) if header_lines else None
    ch_title = title_match.group(1) if title_match else f'Chapter {ch_num}'

    # Extract priority, prereq, postreq from header
    priority = ''
    prereq = ''
    postreq = ''
    for line in header_lines:
        if line.startswith('**优先级**'):
            priority = line
        elif line.startswith('**前置**'):
            prereq = line
        elif line.startswith('**后置**'):
            postreq = line

    # Parse sections
    sections = []
    chapter_level = {}
    current_section = None

    # Section-level keywords (not book sections)
    chapter_kw = ['易错清单', '双线提示', '背诵卡', '参考', '章节目标', '章节链路',
                  '速查', '练习', 'Ch4 vs Ch5', '本章目标', '核心 API',
                  'C 示例摘要', 'Rust 对照', '常见坑与面试点', '示例',
                  '本章完整概述', '双线', '总结', '避坑', '自检',
                  '工业范式', '更多阅读']

    for i in range(first_section_idx or 0, len(lines)):
        line = lines[i]
        match = re.match(r'^##\s+(.+)$', line)
        if match:
            # Save previous section
            if current_section:
                sec_title = current_section['title']
                is_chapter_level = any(kw in sec_title for kw in chapter_kw)
                sec_num_match = re.match(r'^(\d+[\.\-]\d+[a-z]?)', sec_title)
                if not sec_num_match:
                    sec_num_match = re.match(r'^(\d+)\.\s', sec_title)
                if sec_num_match and not is_chapter_level:
                    sections.append(current_section)
                else:
                    chapter_level[current_section['title']] = current_section['content_lines']

            sec_title = match.group(1).strip()
            current_section = {
                'title': sec_title,
                'content_lines': [],
                'start_line': i
            }
        elif current_section:
            current_section['content_lines'].append(line)

    # Don't forget the last section
    if current_section:
        sec_title = current_section['title']
        is_chapter_level = any(kw in sec_title for kw in chapter_kw)
        sec_num_match = re.match(r'^(\d+[\.\-]\d+[a-z]?)', sec_title)
        if not sec_num_match:
            sec_num_match = re.match(r'^(\d+)\.\s', sec_title)
        if sec_num_match and not is_chapter_level:
            sections.append(current_section)
        else:
            chapter_level[current_section['title']] = current_section['content_lines']

    # Process sections: extract section number and content
    processed = []
    for sec in sections:
        title = sec['title']
        content = '\n'.join(sec['content_lines']).strip()

        # Extract section number
        num_match = re.match(r'^(\d+[\.\-]\d+[a-z]?)', title)
        if num_match:
            sec_num = num_match.group(1).replace('-', '.')  # normalize
            # Clean up title (remove number prefix)
            clean_title = re.sub(r'^\d+[\.\-]\d+[a-z]?\s*', '', title).strip()
        else:
            # Non-numbered section (like ch01's "## 1. UNIX...")
            num_match2 = re.match(r'^(\d+)\.', title)
            if num_match2:
                sec_num = f'{ch_num}.{num_match2.group(1)}'
                clean_title = re.sub(r'^\d+\.\s*', '', title).strip()
            else:
                sec_num = ''
                clean_title = title

        if sec_num and clean_title:
            processed.append({
                'num': sec_num,
                'title': clean_title,
                'raw_title': title,
                'content': content,
            })

    return {
        'ch_num': ch_num,
        'ch_title': ch_title,
        'priority': priority,
        'prereq': prereq,
        'postreq': postreq,
        'header': header,
        'sections': processed,
        'chapter_level': chapter_level,
    }

# ========== File Generator ==========

def generate_c_relevance(title, content):
    """Generate a 'C language learning relevance' section."""
    text = (title + ' ' + content).lower()

    # Topic-specific C relevance
    c_topics = {
        'open': '本节涉及的 `open()` 是 C 语言文件操作的基础系统调用。与 K&R 第 8 章的 `fopen()` 不同，`open()` 是 POSIX 系统调用而非 C 标准库函数。理解 `open()` 的 flags 参数（O_RDONLY、O_CREAT 等）是掌握 Unix 系统编程的第一步。对照 K&R 8.3 节的 `open/creat/close` 学习，注意 C 标准库 `fopen` 的 "r"/"w"/"a" 模式如何映射到 `open` 的 flags。',
        'read': '本节的 `read()` 是 K&R 第 8 章（8.2 节）的核心系统调用。K&R 用 `read(fd, buf, n)` 演示了低级 I/O。与 C 标准库 `fread()` 不同，`read()` 无缓冲、直接系统调用。理解短读（short read）是 C 系统编程的基本功——K&R 的 copy.c 示例就利用了 `read` 返回实际字节数的特性。',
        'write': '本节的 `write()` 对应 K&R 8.2 节。K&R 的 copy.c 用 `write(1, buf, n)` 输出到 stdout。注意 `write` 与 `printf`/`fprintf` 的区别：`write` 是无格式化的字节流输出，`printf` 有格式化和缓冲。HFT 场景中 `write` 比 `printf` 更可控。',
        'close': '本节涉及 `close()`，对应 K&R 8.3 节。C 语言中资源管理靠手动——没有 C++ 的 RAII。`close` 必须在所有代码路径上调用，这是 C 语言内存/资源管理的核心训练点。',
        'lseek': '本节的 `lseek()` 对应 K&R 8.4 节（随机访问 lseek）。K&R 用 `lseek` 演示了文件的随机读写。理解 `SEEK_SET/SEEK_CUR/SEEK_END` 三种偏移方式是 C 文件操作的基本功。',
        'fork': '`fork()` 是 C 语言系统编程的核心。C 语言没有内置的并发原语——`fork` 是 Unix 创建进程的唯一方式。理解 `fork` 的返回值（父返回子 PID，子返回 0）需要扎实的 C 语言指针和返回值概念。',
        'exec': '`exec` 家族与 `fork` 配合是 Unix 进程模型的基石。C 语言的 `main` 函数参数 `argc/argv` 在 `exec` 时传入。理解 `execvp` 如何搜索 PATH 需要理解 C 字符串数组和 NULL 终止约定。',
        'malloc': '`malloc/free` 是 C 语言动态内存管理的核心。K&R 第 6 章和第 8.7 节有 `malloc` 的实现。理解 `malloc` 返回 `void*` 需要 C 语言类型系统的知识。`realloc` 的指针失效是 C 语言指针最常见的坑之一。',
        'mmap': '`mmap` 是 C 语言高级内存操作。理解 `mmap` 需要 C 语言指针、内存对齐、页概念的知识。`mmap` 返回 `void*`（不是 NULL）失败是 C 语言中不同于 `malloc` 的一个细节。',
        'signal': '信号处理是 C 语言异步编程的核心。`signal`/`sigaction` 的 handler 是函数指针——需要理解 C 语言的函数指针。`volatile sig_atomic_t` 涉及 C 语言的 `volatile` 关键字和原子性概念。',
        'kill': '`kill` 发送信号看似简单，但涉及 C 语言的进程权限模型。`kill(pid, 0)` 的探测用法需要理解 C 语言的返回值约定（0 成功，-1 失败）和 errno 机制。',
        'pthread': 'Pthreads 是 C 语言多线程编程的标准。`pthread_create` 接收函数指针和 `void*` 参数——这是 C 语言回调模式的核心。线程返回 `void*` 需要 C 语言指针和类型转换的知识。',
        'mutex': '互斥量保护共享数据是 C 并发编程的基本模式。`pthread_mutex_t` 是 C 语言全局变量的典型用法。理解为什么需要 `volatile` + `mutex`（或只用 mutex）需要 C 语言内存模型的初步知识。',
        'pipe': '管道是 C 语言进程间通信的基础。`pipe(fds)` 中 `fds` 是 `int[2]`——需要理解 C 语言的数组传递（退化为指针）。父子进程各自关闭不用的端是 C 语言资源管理的训练。',
        'socket': 'Socket 编程是 C 语言网络编程的核心。`socket/bind/listen/accept` 的调用顺序需要理解 C 语言的 struct（如 `sockaddr_in`）、字节序（`htons/ntohs`）和字符串处理。',
        'epoll': '`epoll` 是 Linux 高性能 I/O 的核心。理解 `epoll_event` 结构体和 `EPOLLIN/EPOLLOUT` 标志需要 C 语言的 struct、位运算和文件描述符知识。ET vs LT 模式的差异直接影响 C 代码的读写循环结构。',
    }

    for kw, relevance in c_topics.items():
        if kw in text:
            return relevance

    # Generic C relevance
    return (f'本节内容属于 Unix 系统编程，是 C 语言学习的进阶实践。'
            f'掌握这些 API 需要扎实的 C 语言基础：指针操作、结构体、'
            f'错误处理（errno/返回值检查）、内存管理。'
            f'建议对照 K&R 和 C 专家编程的相关章节学习，'
            f'将系统 API 的使用与 C 语言底层机制（如函数调用约定、'
            f'栈布局、内存对齐）联系起来理解。')


def generate_man_refs(title, content):
    """Generate man page references based on section content."""
    text = title + ' ' + content
    refs = []

    # Extract API names from backticks
    apis = re.findall(r'`([a-z][a-z0-9_]+)\s*\(?', text)
    seen = set()
    for api in apis:
        if api in seen or len(api) < 2:
            continue
        seen.add(api)
        # Determine man section
        if api in ('open','read','write','close','lseek','ioctl','dup','dup2',
                    'fcntl','mmap','munmap','mprotect','mlock','brk','sbrk',
                    'fork','vfork','clone','execve','exit','_exit','wait',
                    'waitpid','kill','signal','sigaction','sigprocmask',
                    'pipe','socket','bind','listen','accept','connect',
                    'send','recv','sendto','recvfrom','select','poll',
                    'epoll_create','epoll_ctl','epoll_wait','setuid','setgid',
                    'getrlimit','setrlimit','nice','sched_setscheduler',
                    'opendir','readdir','stat','fstat','lstat','chmod','chown',
                    'link','unlink','symlink','mount','nanosleep','clock_gettime',
                    'getpid','getppid','setjmp','longjmp','getenv','setenv'):
            refs.append(f'- `man 2 {api}`')
        elif api in ('printf','fprintf','sprintf','snprintf','scanf','sscanf',
                      'fopen','fclose','fread','fwrite','fgets','fputs','fgetc',
                      'fputc','feof','ferror','perror','strerror','malloc','free',
                      'calloc','realloc','atoi','atol','strtol','strtod',
                      'strcpy','strncpy','strcat','strncat','strcmp','strncmp',
                      'strlen','strchr','strstr','memcpy','memmove','memset',
                      'qsort','bsearch','rand','srand','exit','atexit','abort',
                      'system','getenv','setenv','time','ctime','strftime'):
            refs.append(f'- `man 3 {api}`')
        elif api in ('signal','sigaction','kill','sigprocmask','sigpending',
                      'sigemptyset','sigfillset','sigaddset','sigdelset',
                      'sigismember','alarm','pause'):
            if api not in ('signal','sigaction','kill','sigprocmask','sigpending',
                           'alarm','pause'):
                refs.append(f'- `man 3 {api}`')
        if len(refs) >= 6:
            break

    if not refs:
        refs.append(f'- `man 2` / `man 3` — 查阅本节涉及的系统调用和库函数')
        refs.append('- `man 7` — 查阅概述类页面（如 `man 7 signal`、`man 7 ip`）')

    return '\n'.join(refs)


def generate_section_file(sec, ch_info, prev_sec, next_sec):
    """Generate a section file with the standard skeleton."""
    sec_num = sec['num']
    title = sec['title']
    content = sec['content']
    ch_num = ch_info['ch_num']

    # Slugify for filename
    slug = slugify(title, sec_num)
    filename = f'{sec_num}-{slug}.md'

    # Find matching KB entry
    kb_match = find_kb_match(title, content)
    if kb_match:
        pitfalls, questions = kb_match
    else:
        pitfalls, questions = GENERIC_PITFALLS, GENERIC_QUESTIONS

    # Extract chapter-level content for HFT section
    hft_content = ''
    for key, val in ch_info['chapter_level'].items():
        if '双线' in key or 'HFT' in key:
            if isinstance(val, list):
                hft_content = '\n'.join(val)
            else:
                hft_content = val
            break

    # Extract 易错清单
    pitfall_list = ''
    for key, val in ch_info['chapter_level'].items():
        if '易错' in key:
            if isinstance(val, list):
                pitfall_list = '\n'.join(val)
            else:
                pitfall_list = val
            break

    # Build navigation
    prev_link = f'- 上一节：[{prev_sec["num"]} {prev_sec["title"]}](./{prev_sec["num"]}-{slugify(prev_sec["title"], prev_sec["num"])}.md)' if prev_sec else '- 上一节：本章首页'
    next_link = f'- 下一节：[{next_sec["num"]} {next_sec["title"]}](./{next_sec["num"]}-{slugify(next_sec["title"], next_sec["num"])}.md)' if next_sec else '- 下一节：[下一章](../chapter-{int(ch_num)+1:02d}-*/notes.md)'

    # Build the file
    parts = []

    # Title
    parts.append(f'# {sec_num} {title}')
    parts.append('')

    # Metadata
    parts.append(f'> 本章：[{ch_info["ch_title"]}](./README.md)')
    parts.append(f'> {prev_link}')
    parts.append(f'> {next_link}')
    parts.append('')
    parts.append('---')
    parts.append('')

    # 本节讲什么
    parts.append('## 本节讲什么')
    parts.append('')
    # Generate a brief intro from the section content
    intro_lines = []
    for line in content.split('\n'):
        line = line.strip()
        if not line or line.startswith('```') or line.startswith('|') or line.startswith('###'):
            continue
        if line.startswith('>'):
            continue
        intro_lines.append(line)
        if len(intro_lines) >= 3:
            break
    if intro_lines:
        parts.append(' '.join(intro_lines))
    else:
        parts.append(f'本节讨论 {title} 相关的概念和 API。')
    parts.append('')
    parts.append('---')
    parts.append('')

    # 要点 (existing content)
    parts.append('## 要点')
    parts.append('')
    if content:
        parts.append(content)
    else:
        parts.append(f'（本节核心内容待补充）')
    parts.append('')
    parts.append('---')
    parts.append('')

    # 与 C 语言学习的关系
    parts.append('## 与 C 语言学习的关系')
    parts.append('')
    # Generate C-relevance content based on section keywords
    c_relevance = generate_c_relevance(title, content)
    parts.append(c_relevance)
    parts.append('')
    parts.append('---')
    parts.append('')

    # HFT / 嵌入式关联
    parts.append('## HFT / 嵌入式关联')
    parts.append('')
    if hft_content:
        # Extract relevant parts
        for line in hft_content.split('\n'):
            if line.strip() and not line.startswith('---'):
                parts.append(line)
    else:
        parts.append(f'| 路线 | 要点 |')
        parts.append(f'|------|------|')
        parts.append(f'| 嵌入式 | {title} 在嵌入式系统的应用场景 |')
        parts.append(f'| HFT | {title} 对低延迟和性能的影响 |')
    parts.append('')
    parts.append('---')
    parts.append('')

    # 常见陷阱
    parts.append('## 常见陷阱')
    parts.append('')
    for i, (ptitle, pcode, pexpl) in enumerate(pitfalls, 1):
        parts.append(f'{i}. **{ptitle}**')
        parts.append('')
        parts.append(pcode)
        parts.append('')
        parts.append(pexpl)
        parts.append('')
    parts.append('---')
    parts.append('')

    # 自测题
    parts.append('## 自测题')
    parts.append('')
    for i, (qtype, qtext, qanswer) in enumerate(questions, 1):
        parts.append(f'**题目 {i}（{qtype}）：** {qtext}')
        parts.append('')
        parts.append('<details>')
        parts.append('<summary>参考答案</summary>')
        parts.append('')
        parts.append(qanswer)
        parts.append('')
        parts.append('</details>')
        parts.append('')
    parts.append('---')
    parts.append('')

    # 衔接
    parts.append('## 衔接')
    parts.append('')
    parts.append(f'- 本章 README：[README](./README.md)')
    if prev_sec:
        prev_slug = slugify(prev_sec['title'], prev_sec['num'])
        parts.append(f'- 上一节：[{prev_sec["num"]} {prev_sec["title"]}](./{prev_sec["num"]}-{prev_slug}.md)')
    if next_sec:
        next_slug = slugify(next_sec['title'], next_sec['num'])
        parts.append(f'- 下一节：[{next_sec["num"]} {next_sec["title"]}](./{next_sec["num"]}-{next_slug}.md)')
    # Add chapter-level links
    if ch_info['prereq']:
        parts.append(f'- 前置：{ch_info["prereq"]}')
    if ch_info['postreq']:
        parts.append(f'- 后置：{ch_info["postreq"]}')
    parts.append('')
    parts.append('---')
    parts.append('')

    # man 手册参考
    parts.append('## man 手册参考')
    parts.append('')
    man_refs = generate_man_refs(title, content)
    parts.append(man_refs)
    parts.append('')

    return filename, '\n'.join(parts)


def generate_readme(ch_info, sections):
    """Generate a README.md for the chapter."""
    parts = []

    ch_num = ch_info['ch_num']
    ch_title = ch_info['ch_title']

    parts.append(f'# {ch_title}')
    parts.append('')

    # Metadata
    if ch_info['priority']:
        parts.append(ch_info['priority'])
    if ch_info['prereq']:
        parts.append(ch_info['prereq'])
    if ch_info['postreq']:
        parts.append(ch_info['postreq'])
    parts.append('')
    parts.append('---')
    parts.append('')

    # Section index
    parts.append('## 小节目录')
    parts.append('')
    for sec in sections:
        slug = slugify(sec['title'], sec['num'])
        parts.append(f'- [{sec["num"]} {sec["title"]}](./notes/{sec["num"]}-{slug}.md)')
    parts.append('')
    parts.append('---')
    parts.append('')

    # Chapter-level content (易错清单, 背诵卡, etc.)
    for key, val in ch_info['chapter_level'].items():
        parts.append(f'## {key}')
        parts.append('')
        if isinstance(val, list):
            parts.append('\n'.join(val))
        else:
            parts.append(val)
        parts.append('')
        parts.append('---')
        parts.append('')

    # Reference
    parts.append('## 参考')
    parts.append('')
    parts.append(f'- [OUTLINE](../OUTLINE.md)')
    parts.append(f'- 原始笔记：[notes.md.bak](./notes.md.bak)')
    parts.append('')

    return '\n'.join(parts)


# ========== Main ==========

def main():
    chapter_dirs = sorted(glob.glob(os.path.join(BASE, 'chapter-*')))

    stats = {'chapters': 0, 'sections': 0, 'lines': 0}

    for ch_dir in chapter_dirs:
        notes_path = os.path.join(ch_dir, 'notes.md')
        if not os.path.exists(notes_path):
            print(f'SKIP (no notes.md): {os.path.basename(ch_dir)}')
            continue

        ch_info = parse_notes_md(notes_path)
        if not ch_info['sections']:
            print(f'SKIP (no sections): {os.path.basename(ch_dir)}')
            continue

        # Create notes/ subdirectory
        notes_dir = os.path.join(ch_dir, 'notes')
        os.makedirs(notes_dir, exist_ok=True)

        # Generate section files
        sections = ch_info['sections']
        for i, sec in enumerate(sections):
            prev_sec = sections[i-1] if i > 0 else None
            next_sec = sections[i+1] if i < len(sections)-1 else None

            filename, content = generate_section_file(sec, ch_info, prev_sec, next_sec)
            filepath = os.path.join(notes_dir, filename)
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)

            line_count = content.count('\n') + 1
            stats['lines'] += line_count
            stats['sections'] += 1

        # Generate README
        readme_content = generate_readme(ch_info, sections)
        readme_path = os.path.join(ch_dir, 'README.md')
        # Don't overwrite existing README if it exists
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(readme_content)

        # Rename original notes.md to .bak
        bak_path = notes_path + '.bak'
        os.rename(notes_path, bak_path)

        stats['chapters'] += 1
        print(f'OK: {os.path.basename(ch_dir)} - {len(sections)} sections')

    # Also process extras
    for extra_dir in sorted(glob.glob(os.path.join(BASE, 'extras-*'))):
        notes_path = os.path.join(extra_dir, 'notes.md')
        if not os.path.exists(notes_path):
            continue

        ch_info = parse_notes_md(notes_path)
        if not ch_info['sections']:
            print(f'SKIP (no sections): {os.path.basename(extra_dir)}')
            continue

        notes_dir = os.path.join(extra_dir, 'notes')
        os.makedirs(notes_dir, exist_ok=True)

        sections = ch_info['sections']
        for i, sec in enumerate(sections):
            prev_sec = sections[i-1] if i > 0 else None
            next_sec = sections[i+1] if i < len(sections)-1 else None

            filename, content = generate_section_file(sec, ch_info, prev_sec, next_sec)
            filepath = os.path.join(notes_dir, filename)
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)

            stats['lines'] += content.count('\n') + 1
            stats['sections'] += 1

        readme_content = generate_readme(ch_info, sections)
        readme_path = os.path.join(extra_dir, 'README.md')
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(readme_content)

        os.rename(notes_path, notes_path + '.bak')
        print(f'OK: {os.path.basename(extra_dir)} - {len(sections)} sections')

    print(f'\n=== Stats ===')
    print(f'Chapters: {stats["chapters"]}')
    print(f'Sections: {stats["sections"]}')
    print(f'Total lines: {stats["lines"]}')
    print(f'Avg lines/section: {stats["lines"]/max(stats["sections"],1):.1f}')

if __name__ == '__main__':
    main()
