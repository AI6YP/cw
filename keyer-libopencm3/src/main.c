#include <stdlib.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include "hid.h"

static usbd_device *usbd_dev;

#define FALLING 0
#define RISING 1

uint16_t exti_direction = FALLING;

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes hid_control_request(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)
) {
    (void)complete;
    (void)dev;

    if ((req->bmRequestType != 0x81) ||
        (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
        (req->wValue != 0x2200)
    ) {
        return USBD_REQ_NOTSUPP;
    }

    /* Handle the HID report descriptor. */
    *buf = (uint8_t *)hid_report_descriptor;
    *len = sizeof(hid_report_descriptor);

    return USBD_REQ_HANDLED;
}

#ifdef INCLUDE_DFU_INTERFACE
static void dfu_detach_complete(
    usbd_device *dev,
    struct usb_setup_data *req
) {
    (void)req;
    (void)dev;

    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO10
    );
    gpio_set(GPIOA, GPIO10);
    scb_reset_core();
}

static enum usbd_request_return_codes dfu_control_request(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)
) {
    (void)buf;
    (void)len;
    (void)dev;

    if ((req->bmRequestType != 0x21) || (req->bRequest != DFU_DETACH)) {
        return USBD_REQ_NOTSUPP; /* Only accept class request. */
    }

    *complete = dfu_detach_complete;

    return USBD_REQ_HANDLED;
}
#endif

static void hid_set_config(usbd_device *dev, uint16_t wValue) {
    (void)wValue;
    (void)dev;

    usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, 0);

    usbd_register_control_callback(
        dev,
        USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        hid_control_request
    );

#ifdef INCLUDE_DFU_INTERFACE
    usbd_register_control_callback(
        dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        dfu_control_request
    );
#endif

    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    /* SysTick interrupt every N clock pulses: set reload to N-1 */
    // systick_set_reload(99999);
    // systick_interrupt_enable();
    // systick_counter_enable();
}

static void gpio_setup(void) {
    /* Enable GPIOC clock. */

    rcc_clock_setup_in_hsi_out_48mhz();

    rcc_periph_clock_enable(RCC_GPIOC); // LED

    /* Set GPIO13 (LED1) to 'output push-pull'. */
    gpio_set_mode(
        GPIOC,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO13
    );

    for (int i = 0; i < 8; i++) {
        gpio_toggle(GPIOC, GPIO13);
        for (unsigned i = 0; i < 800000; i++) {
            __asm__("nop");
        }
    }

    rcc_periph_clock_enable(RCC_GPIOA); // USB

    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO12
    );

    gpio_clear(GPIOA, GPIO12);

    for (unsigned i = 0; i < 800000; i++) {
        __asm__("nop");
    }

}

static void exti_setup(void)
{
    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOA);

    /* Enable AFIO clock. */
    rcc_periph_clock_enable(RCC_AFIO);

    /* Enable EXTI0 interrupt. */
    nvic_enable_irq(NVIC_EXTI0_IRQ);

    /* Set PA0 to 'input float'. */
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_INPUT,
        GPIO_CNF_OUTPUT_OPENDRAIN,
        GPIO0
    );
    /* Set PA3 to '0' */
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO3
    );

    /* Configure the EXTI subsystem. */
    exti_select_source(EXTI0, GPIOA);
    exti_direction = FALLING;
    exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
    exti_enable_request(EXTI0);
}

void exti0_isr(void) {
    exti_reset_request(EXTI0);

    // TODO send keyboard report
    // uint8_t buf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    //
    // usbd_ep_write_packet(usbd_dev, 0x81, buf, 4);

    if (exti_direction == FALLING) {
        gpio_set(GPIOC, GPIO13);
        exti_direction = RISING;
        exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    } else {
        gpio_clear(GPIOC, GPIO13);
        exti_direction = FALLING;
        exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
    }
}

int main (void) {
    gpio_setup();
    exti_setup();

    usbd_dev = usbd_init(
        &st_usbfs_v1_usb_driver,
        &dev_descr,
        &config,
        usb_strings,
        3,
        usbd_control_buffer,
        sizeof(usbd_control_buffer)
    );

    usbd_register_set_config_callback(usbd_dev, hid_set_config);

    while (1) {
        usbd_poll(usbd_dev);
        // gpio_toggle(GPIOC, GPIO13);
        // GPIOC_BSRR = GPIO13;
        // for (int i = 0; i < 200000; i++) { __asm__("nop"); }
        // GPIOC_BRR = GPIO13;
        // for (int i = 0; i < 200000; i++) { __asm__("nop"); }
    }
    return 0;
}

void sys_tick_handler(void) {
    static int x = 0;
    static int dir = 1;
    uint8_t buf[4] = {0, 0, 0, 0};

    buf[1] = dir;
    x += dir;
    if (x > 30)
        dir = -dir;
    if (x < -30)
        dir = -dir;

    usbd_ep_write_packet(usbd_dev, 0x81, buf, 4);
}
