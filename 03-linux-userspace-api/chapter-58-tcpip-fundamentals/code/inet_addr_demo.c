#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *ipstr;
    unsigned port;
    struct sockaddr_in addr;
    char buf[INET_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr, "usage: %s <ipv4> <port>\n", argv[0]);
        return 1;
    }
    ipstr = argv[1];
    port = (unsigned)atoi(argv[2]);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, ipstr, &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton: bad address '%s'\n", ipstr);
        return 1;
    }

    if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)) == NULL) {
        perror("inet_ntop");
        return 1;
    }

    printf("family=AF_INET\n");
    printf("presentation: %s:%u\n", buf, (unsigned)ntohs(addr.sin_port));
    printf("sin_port (net)=0x%04x  s_addr (net)=0x%08x\n",
           (unsigned)addr.sin_port, (unsigned)addr.sin_addr.s_addr);
    printf("INADDR_ANY net=0x%08x (server bind all ifaces)\n",
           (unsigned)htonl(INADDR_ANY));
    return 0;
}
