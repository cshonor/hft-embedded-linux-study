#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int is_little_endian(void)
{
    uint16_t x = 0x0102;
    return *(uint8_t *)&x == 0x02;
}

int main(void)
{
    uint8_t buf[4] = { 0x12, 0x34, 0x56, 0x78 };
    uint32_t raw;
    uint32_t host = 0x12345678;

    memcpy(&raw, buf, 4);

    printf("host endian: %s\n", is_little_endian() ? "little" : "big");
    printf("*(uint32_t*)buf = 0x%08x (depends on host endian)\n", raw);
    printf("htonl(0x12345678) = 0x%08x (network byte order)\n", htonl(host));
    printf("ntohl(htonl(host)) = 0x%08x\n", ntohl(htonl(host)));

    return 0;
}
