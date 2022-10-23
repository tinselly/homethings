

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "strip.h"

/******************************************************************************/

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#define DELAY_TIME K_MSEC(50)

/******************************************************************************/

#define RGB(rgb) \
    ((struct led_rgb) { .r = ((rgb >> 16) & 0xFF), .g = ((rgb >> 8) & 0xFF), .b = ((rgb >> 0) & 0xFF) })

/******************************************************************************/

LOG_MODULE_REGISTER(strip);

K_MUTEX_DEFINE(s_strip_mutex);

/******************************************************************************/

static struct led_rgb s_color_palette[STRIP_COLOR_PALETTE_SIZE] = {
    RGB(0x450000),
};

static uint32_t s_color_palette_size = 1;
static uint32_t s_speed_ms = 500;

static struct led_rgb s_pixels[STRIP_NUM_PIXELS];
static const struct device* const s_strip = DEVICE_DT_GET(STRIP_NODE);

/******************************************************************************/

static void strip_thread(void* unused1, void* unused2, void* unused3)
{

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    size_t cursor = 0, color = 0;
    int rc;

    if (device_is_ready(s_strip)) {
        LOG_INF("Found LED strip device %s", s_strip->name);
    } else {
        LOG_ERR("LED strip device %s is not ready", s_strip->name);
        return;
    }

    uint32_t count = 0;
    LOG_INF("Displaying pattern on strip");
    while (1) {
        memset(&s_pixels, 0x00, sizeof(s_pixels));

        for (size_t i = 0; i < ARRAY_SIZE(s_pixels); ++i) {
            memcpy(&s_pixels[i], &s_color_palette[color], sizeof(struct led_rgb));
        }

        rc = led_strip_update_rgb(s_strip, s_pixels, STRIP_NUM_PIXELS);

        if (rc) {
            LOG_ERR("couldn't update strip: %d", rc);
        }

        cursor++;
        if (cursor >= STRIP_NUM_PIXELS) {
            cursor = 0;
            color++;
            if (color == ARRAY_SIZE(s_color_palette)) {
                color = 0;
            }
        }

        k_sleep(K_FOREVER);
    }
}

/******************************************************************************/

void strip_set_color_palette(size_t i, uint32_t color)
{
    if (i >= ARRAY_SIZE(s_color_palette)) {
        return;
    }

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_color_palette[i] = RGB(color);

    k_mutex_unlock(&s_strip_mutex);
}

/******************************************************************************/

K_THREAD_DEFINE(s_strip_thread, 2048, strip_thread, NULL, NULL, NULL, 1, 0, 0);

/******************************************************************************/
