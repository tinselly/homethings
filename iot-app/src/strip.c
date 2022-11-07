

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

#define STRIP_FPS 64
#define STRIP_FIXED_FRAME_MS (1000 / STRIP_FPS)

/******************************************************************************/

LOG_MODULE_REGISTER(strip);

K_MUTEX_DEFINE(s_strip_mutex);

static void strip_thread(void* unused1, void* unused2, void* unused3);

K_THREAD_DEFINE(s_strip_thread, 2048, strip_thread, NULL, NULL, NULL, -1, 0, 0);

/******************************************************************************/

static struct strip_config s_config;
static struct strip_state s_state;

static uint32_t s_anim_time_ms = 5000;
static uint32_t s_cursor = 0;
static uint32_t s_cursor_next = 1;

static struct led_rgb s_pixels[STRIP_NUM_PIXELS];
static const struct device* const s_strip = DEVICE_DT_GET(STRIP_NODE);

/******************************************************************************/

static void led_rgb_set_color(color_t color, struct led_rgb* rgb) {
    rgb->r = (color >> 16) & 0xFF;
    rgb->g = (color >> 8) & 0xFF;
    rgb->b = (color >> 0) & 0xFF;
}

/******************************************************************************/

static void strip_finished() {
    s_state.color_prev = (s_state.color_prev + 1) % s_config.colors_count;
    s_state.color_next = (s_state.color_prev + 1) % s_config.colors_count;
}

static void strip_animate(uint32_t dt, uint32_t time) {
    struct led_rgb color;

    color_t result = color_lerp(s_config.colors[s_state.color_prev],
                                s_config.colors[s_state.color_next],
                                (time * 100) / s_anim_time_ms);

    result = color_intensity(result, color_get_alpha(result));
    // result = color_intensity(result, s_config.intensity);

    led_rgb_set_color(result, &color);

    for (size_t i = 0; i < s_config.pixels_count; ++i) {
        memcpy(&s_pixels[i], &color, sizeof(struct led_rgb));
    }
}

static void strip_update(uint32_t dt) {
    /* Reset pixels */
    memset(&s_pixels, 0x00, sizeof(s_pixels));

    s_state.time = MIN(s_anim_time_ms, s_state.time + dt);

    strip_animate(dt, s_state.time);

    if (s_state.time >= s_anim_time_ms) {
        s_state.time = 1;
        strip_finished();
    }

    /* Update led strip */
    led_strip_update_rgb(s_strip, &s_pixels[0], ARRAY_SIZE(s_pixels));
}

void strip_thread(void* unused1, void* unused2, void* unused3) {

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    k_thread_cpu_pin(s_strip_thread, 0);

    if (!device_is_ready(s_strip)) {
        LOG_ERR("LED strip device %s is not ready", s_strip->name);
        return;
    }

    k_mutex_lock(&s_strip_mutex, K_FOREVER);

    memset(&s_config, 0x00, sizeof(s_config));

    s_config.colors[s_config.colors_count++] = 0xffdb920b;
    s_config.colors[s_config.colors_count++] = 0xffe6b800;
    s_config.colors[s_config.colors_count++] = 0xffe6c807;
    s_config.colors[s_config.colors_count++] = 0xffcfd60b;
    s_config.intensity = 85;
    s_config.speed = 100;
    s_config.pixels_count = ARRAY_SIZE(s_pixels);

    memset(&s_state, 0x00, sizeof(s_state));
    s_state.config = &s_config;
    s_state.time = 0;

    k_mutex_unlock(&s_strip_mutex);

    uint32_t prev_time = 0;
    while (true) {

        const uint32_t start_time = k_uptime_get_32();
        const uint32_t dt_time = start_time - prev_time;
        prev_time = start_time;

        k_mutex_lock(&s_strip_mutex, K_FOREVER);
        strip_update(dt_time);
        k_mutex_unlock(&s_strip_mutex);

        // Keep updating at fixed frame rate
        const uint32_t diff_time = k_uptime_get_32() - start_time;

        if (diff_time < STRIP_FIXED_FRAME_MS) {
            k_msleep(STRIP_FIXED_FRAME_MS - diff_time);
        } else {
            k_msleep(1);
        }
    }
}

/******************************************************************************/

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

void strip_set_animation(uint8_t animation) {}

/******************************************************************************/
