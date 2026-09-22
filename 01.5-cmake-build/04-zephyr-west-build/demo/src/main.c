#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 设备树（09 模块）给出的 LED 节点：硬件描述与代码分离，
 * 这块板有没有 led0、接在哪个引脚，全由 .dts 决定，代码不写死 */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	if (!gpio_is_ready_dt(&led))
		return -1;
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	while (1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(500);   /* Zephyr 内核 API：睡 500ms，不是 POSIX sleep */
	}
	return 0;
}
