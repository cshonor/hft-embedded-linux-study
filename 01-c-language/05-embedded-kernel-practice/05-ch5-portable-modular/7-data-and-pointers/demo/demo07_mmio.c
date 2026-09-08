#include <stdio.h>
#include <stdint.h>

/* 模拟 STM32 风格 UART 寄存器块（纯用户态内存） */
typedef struct {
    volatile uint32_t DR;
    volatile uint32_t SR;
    volatile uint32_t CR1;
} uart_regs_t;

static uart_regs_t fake_uart;

#define UART_BASE ((uart_regs_t *)0) /* 占位；main 中用 &fake_uart */

static void uart_putc(uart_regs_t *u, char c)
{
    u->DR = (uint32_t)c;
    u->SR |= 0x1; /* TX ready */
}

static char uart_getc(uart_regs_t *u)
{
    while (!(u->SR & 0x2))
        ; /* RX not empty — busy wait */
    return (char)(u->DR & 0xFF);
}

int main(void)
{
    uart_regs_t *uart = &fake_uart;
    fake_uart.SR = 0x2;
    fake_uart.DR = 'A';

    printf("MMIO struct @ %p  DR=%#x SR=%#x\n",
           (void *)uart, uart->DR, uart->SR);

    uart_putc(uart, '!');
    printf("after putc: DR=%#x\n", uart->DR);

    char c = uart_getc(uart);
    printf("getc -> '%c'\n", c);

    puts("真硬件: #define UART_DR (*(volatile uint32_t *)0x40000000)");
    (void)UART_BASE;
    return 0;
}
