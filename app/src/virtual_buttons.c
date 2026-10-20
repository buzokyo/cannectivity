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

struct virtual_button { struct gpio_dt_spec gpio; uint8_t bit; };
static const struct virtual_button buttons[] = {
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button0))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button0), gpios), BIT(0) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button1))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button1), gpios), BIT(1) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button2))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button2), gpios), BIT(2) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button3))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button3), gpios), BIT(3) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button4))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button4), gpios), BIT(4) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button5))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button5), gpios), BIT(5) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button6))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button6), gpios), BIT(6) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(yfa_button7))
    { GPIO_DT_SPEC_GET(DT_ALIAS(yfa_button7), gpios), BIT(7) },
#endif
    { {0}, 0 }, /* sentinel permits zero configured aliases */
};
#define BUTTON_COUNT (ARRAY_SIZE(buttons) - 1)

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
        err = gpio_pin_get_dt(&buttons[i].gpio);
        if (err > 0) {
            sampled |= buttons[i].bit;
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
        if (!gpio_is_ready_dt(&buttons[i].gpio)) {
            return -ENODEV;
        }
        int err = gpio_pin_configure_dt(&buttons[i].gpio, GPIO_INPUT | GPIO_PULL_UP);
        if (err != 0) {
            return err;
        }
    }
    usb_dev = gs_usb;
    k_work_init_delayable(&poll_work, poll_buttons);
    return k_work_schedule(&poll_work, K_MSEC(10)) < 0 ? -EIO : 0;
}
