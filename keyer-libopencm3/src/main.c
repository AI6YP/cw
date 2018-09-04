#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <hid.h>

static usbd_device *usbd_dev;

static void gpio_setup(void)
{
    /* Enable GPIOC clock. */
    rcc_periph_clock_enable(RCC_GPIOC);

    /* Set GPIO13 (LED1) to 'output push-pull'. */
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

int main (void) {
    gpio_setup();
    while (1) {
        gpio_toggle(GPIOC, GPIO13);
        for (int i = 0; i < 80000; i++) {
            __asm__("nop");
        }
    }
    return 0;
}
