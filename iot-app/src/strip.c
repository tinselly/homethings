

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

#define STRIP_FPS 600
#define STRIP_FIXED_FRAME_MS (1000 / STRIP_FPS)

/******************************************************************************/

LOG_MODULE_REGISTER(strip);

K_MUTEX_DEFINE(s_strip_mutex);

static void strip_thread(void* unused1, void* unused2, void* unused3);

K_THREAD_DEFINE(s_strip_thread, 3072, strip_thread, NULL, NULL, NULL, -1, 0, 0);

/******************************************************************************/

static uint32_t s_colors[STRIP_COLOR_MAX_COUNT] = {
    0x170f01,
    0x1a0a00,
    0x140401,
};

static uint32_t s_color_count = 3;
static uint32_t s_anim_time_ms = 70000;
static uint32_t s_time_ms = 0;
static uint32_t s_cursor = 0;
static uint32_t s_cursor_next = 1;
static uint32_t s_intensity = 100;

static struct led_rgb s_pixels[STRIP_NUM_PIXELS];
static const struct device* const s_strip = DEVICE_DT_GET(STRIP_NODE);

/******************************************************************************/

static uint8_t color_lerp_comp(uint8_t lhs, uint8_t rhs, uint8_t alpha)
{
    return lhs + (uint8_t)(((float)alpha * (float)(rhs - lhs)) / 100.0f);
}

/**
 * @brief
 *
 * @param lhs
 * @param rhs
 * @param alpha 0 - 100 %
 * @return Result color
 */
static uint32_t color_lerp(uint32_t lhs, uint32_t rhs, uint8_t alpha)
{
    uint32_t result = 0;

    alpha = MIN(100, alpha);

    result += color_lerp_comp((lhs >> 16) & 0xFF, (rhs >> 16) & 0xFF, alpha) << 16;
    result += color_lerp_comp((lhs >> 8) & 0xFF, (rhs >> 8) & 0xFF, alpha) << 8;
    result += color_lerp_comp((lhs >> 0) & 0xFF, (rhs >> 0) & 0xFF, alpha) << 0;

    return result;
}

static uint8_t color_intensity_comp(uint8_t c, uint8_t intensity)
{
    return (uint8_t)(((int)c * (int)intensity) / 100);
}

static uint32_t color_intensity(uint32_t color, uint8_t intensity)
{
    uint32_t result = 0;

    intensity = MIN(100, intensity);

    result += color_intensity_comp((color >> 16) & 0xFF, intensity) << 16;
    result += color_intensity_comp((color >> 8) & 0xFF, intensity) << 8;
    result += color_intensity_comp((color >> 0) & 0xFF, intensity) << 0;

    return result;
}

static void set_led_rgb(uint32_t color, struct led_rgb* rgb)
{
    rgb->r = (color >> 16) & 0xFF;
    rgb->g = (color >> 8) & 0xFF;
    rgb->b = (color >> 0) & 0xFF;
}

/******************************************************************************/

static void strip_finished()
{
    s_cursor = (s_cursor + 1) % s_color_count;
    s_cursor_next = (s_cursor + 1) % s_color_count;
}

static void strip_animate(uint32_t dt, uint32_t time)
{
    struct led_rgb color;

    const uint32_t lerp = color_intensity(
        color_lerp(s_colors[s_cursor], s_colors[s_cursor_next], (time * 100) / s_anim_time_ms),
         s_intensity);

    set_led_rgb(lerp, &color);

    for (size_t i = 0; i < ARRAY_SIZE(s_pixels); ++i) {
        memcpy(&s_pixels[i], &color, sizeof(struct led_rgb));
    }
}

static void strip_update(uint32_t dt)
{
    /* Reset pixels */
    memset(&s_pixels, 0x00, sizeof(s_pixels));

    s_time_ms = MIN(s_anim_time_ms, s_time_ms + dt);

    strip_animate(dt, s_time_ms);

    if (s_time_ms >= s_anim_time_ms) {
        s_time_ms = 1;
        strip_finished();
    }

    /* Update led strip */
    led_strip_update_rgb(s_strip, &s_pixels[0], ARRAY_SIZE(s_pixels));
}

void strip_thread(void* unused1, void* unused2, void* unused3)
{

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    k_thread_cpu_pin(s_strip_thread, 0);

    size_t cursor = 0, color = 0;

    if (!device_is_ready(s_strip)) {
        LOG_ERR("LED strip device %s is not ready", s_strip->name);
        return;
    }

    uint32_t prev_time = 0;
    while (true) {

        const uint32_t start_time = k_uptime_get_32();
        const uint32_t dt_time = start_time - prev_time;
        prev_time = start_time;

        strip_update(dt_time);

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

void strip_set_color_count(size_t count)
{
    if (count > ARRAY_SIZE(s_colors)) {
        return;
    }

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_color_count = MAX(1, count);

    k_mutex_unlock(&s_strip_mutex);
}

void strip_set_color(size_t i, uint32_t color)
{
    if (i >= ARRAY_SIZE(s_colors)) {
        return;
    }

    if (k_mutex_lock(&s_strip_mutex, K_MSEC(500)) != 0) {
        return;
    }

    s_colors[i] = color;

    k_mutex_unlock(&s_strip_mutex);
}

/******************************************************************************/
