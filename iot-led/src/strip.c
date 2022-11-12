

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

/******************************************************************************/

LOG_MODULE_REGISTER(strip);

K_MUTEX_DEFINE(s_strip_mutex);
K_SEM_DEFINE(s_strip_sem, 0, 1);

/******************************************************************************/

static struct led_rgb s_pixels[STRIP_NUM_PIXELS];
static const struct device* const s_strip = DEVICE_DT_GET(STRIP_NODE);

static struct strip_config s_config = {
    .colors = { 0xff7a5a01, 0xff7a5a01 },
    .colors_count = 1,
    .pixels = &s_pixels[0],
    .pixels_count = STRIP_NUM_PIXELS,
    .intensity = 240,
    .animation_time = 3 * 1000,
    .animation = 0,
    .enabled = true,
};

static struct strip_state s_state = {
    .time = 0,
    .color_prev_idx = 0,
    .color_next_idx = 1,
};

/******************************************************************************/

void strip_color_to_rgb(color_t color, struct led_rgb* rgb) {
    rgb->r = (color >> 16) & 0xFF;
    rgb->g = (color >> 8) & 0xFF;
    rgb->b = (color >> 0) & 0xFF;
}

int strip_lock(k_timeout_t timeout) {
    return k_mutex_lock(&s_strip_mutex, timeout);
}

void strip_unlock() {
    k_mutex_unlock(&s_strip_mutex);
}

int strip_wait_for_enabled(k_timeout_t timeout) {
    k_sem_reset(&s_strip_sem);
    return k_sem_take(&s_strip_sem, K_FOREVER);
}

const struct strip_config* strip_get_config() {
    return &s_config;
}

struct strip_config* strip_get_config_mut() {
    return &s_config;
}

const struct strip_state* strip_get_state() {
    return &s_state;
}

struct strip_state* strip_get_state_mut() {
    return &s_state;
}

int strip_sync() {
    return led_strip_update_rgb(s_strip, &s_pixels[0], ARRAY_SIZE(s_pixels));
}

void strip_set_color_count(size_t count) {
    if (count > ARRAY_SIZE(s_config.colors)) {
        return;
    }

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_config.colors_count = MAX(1, count);

    k_mutex_unlock(&s_strip_mutex);
}

void strip_set_color(size_t i, color_t color) {
    if (i >= ARRAY_SIZE(s_config.colors)) {
        return;
    }

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_config.colors[i] = color;

    k_mutex_unlock(&s_strip_mutex);
}

void strip_set_intensity(uint8_t intensity) {
    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_config.intensity = intensity;

    k_mutex_unlock(&s_strip_mutex);
}

void strip_set_animation(uint8_t animation) {

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_config.animation = animation;

    k_mutex_unlock(&s_strip_mutex);
}

void strip_set_enabled(bool enabled) {
    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_config.enabled = enabled;

    if (s_config.enabled) {
        k_sem_give(&s_strip_sem);
    }

    k_mutex_unlock(&s_strip_mutex);
}

/******************************************************************************/
