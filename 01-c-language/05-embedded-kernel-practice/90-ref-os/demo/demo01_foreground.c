#include <stdio.h>
#include <stdint.h>

typedef void (*task_fn)(void);

static volatile uint8_t flag_uart;
static volatile uint8_t flag_led;
static uint32_t tick;

static void task_uart_poll(void)
{
    if (flag_uart) {
        puts("[uart] handle rx byte");
        flag_uart = 0;
    }
}

static void task_led_poll(void)
{
    if (flag_led) {
        printf("[led] blink @ tick=%u\n", tick);
        flag_led = 0;
    }
}

static void isr_simulate_events(void)
{
    if (tick % 3 == 0)
        flag_uart = 1;
    if (tick % 5 == 0)
        flag_led = 1;
}

int main(void)
{
    puts("demo01: foreground/background super-loop (no preemption)");
    for (tick = 1; tick <= 15; tick++) {
        isr_simulate_events();
        task_uart_poll();
        task_led_poll();
    }
    return 0;
}
