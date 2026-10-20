/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual button states are reported over gs_usb RX only. No CAN TX calls.
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <cannectivity/usb/class/gs_usb.h>
#include "cannectivity.h"

LOG_MODULE_REGISTER(virtual_buttons, CONFIG_CANNECTIVITY_LOG_LEVEL);

#define BUTTON_ENTRY(n) IF_ENABLED(DT_NODE_EXISTS(DT_ALIAS(yfa_button##n)), \
    (GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button##n), gpios),))
/* Each alias points to a node with a gpios property, with GPIO_ACTIVE_LOW. */
static const struct gpio_dt_spec buttons[] = {
    BUTTON_ENTRY(0)
    BUTTON_ENTRY(1)
    BUTTON_ENTRY(2)
    BUTTON_ENTRY(3)
    BUTTON_ENTRY(4)
    BUTTON_ENTRY(5)
    BUTTON_ENTRY(6)
    BUTTON_ENTRY(7)
};
#define BUTTON_COUNT ARRAY_SIZE(buttons)

static const struct device *usb_dev;
static struct k_work_delayable poll_work;
static uint8_t stable_state;
static uint8_t last_sample;
static uint8_t sequence;
static uint8_t stable_samples;
static bool initial_pending = true;

static void poll_buttons(struct k_work *work)
{
    uint8_t sampled = 0;
    int err;
    ARG_UNUSED(work);

    for (size_t i = 0; i < BUTTON_COUNT; ++i) {
        err = gpio_pin_get_dt(&buttons[i]);
        if (err > 0) {
            sampled |= BIT(i);
        }
    }

    if (sampled != last_sample) {
        last_sample = sampled;
        stable_samples = 0;
    } else if (stable_samples < 3) {
        stable_samples++;
    }

    if (stable_samples >= 3 && (sampled != stable_state || initial_pending)) {
        uint8_t payload[2] = {sampled, sequence};
        err = gs_usb_inject_rx(usb_dev, 0, 0x555, payload, sizeof(payload));
        if (err == 0) {
            stable_state = sampled;
            initial_pending = false;
            sequence++;
        }
    }

    (void)k_work_reschedule(&poll_work, K_MSEC(10));
}

int cannectivity_virtual_buttons_init(const struct device *gs_usb)
{
    if (BUTTON_COUNT > 8) {
        return -EINVAL;
    }
    for (size_t i = 0; i < BUTTON_COUNT; ++i) {
        if (!gpio_is_ready_dt(&buttons[i])) {
            return -ENODEV;
        }
        int err = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT | GPIO_PULL_UP);
        if (err != 0) {
            return err;
        }
    }
    usb_dev = gs_usb;
    k_work_init_delayable(&poll_work, poll_buttons);
    return k_work_schedule(&poll_work, K_MSEC(10)) < 0 ? -EIO : 0;
}
