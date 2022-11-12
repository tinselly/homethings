

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "animation.h"
#include "strip.h"

/******************************************************************************/

#define STRIP_FPS 90
#define STRIP_FIXED_FRAME_MS (1000 / STRIP_FPS)
#define ANIMATION_MAX_COUNT 16

/******************************************************************************/

void strip_thread(void* unused1, void* unused2, void* unused3);

/******************************************************************************/

LOG_MODULE_REGISTER(strip_thread);

K_THREAD_DEFINE(s_strip_thread, 4096, strip_thread, NULL, NULL, NULL, -1, 0, 0);

/******************************************************************************/

static struct animation s_animations[ANIMATION_MAX_COUNT];

/******************************************************************************/

static void strip_finished(struct strip_state* state, const struct strip_config* config) {

    state->time = 0;

    ++state->color_prev_idx;
    if (state->color_prev_idx >= config->colors_count) {
        state->color_prev_idx = 0;
    }

    state->color_next_idx = 1 + state->color_prev_idx;
    if (state->color_next_idx >= config->colors_count) {
        state->color_next_idx = 0;
    }
}

static void
strip_update(uint32_t dt, struct strip_state* state, const struct strip_config* config) {

    state->time = MIN(config->animation_time, state->time + dt);
    state->animation_dt = ((state->time * 100u) / config->animation_time);

    if (config->animation < ARRAY_SIZE(s_animations) && s_animations[config->animation].animate) {
        s_animations[config->animation].animate(state, config, s_animations[config->animation].ctx);
    }

    if (state->time >= config->animation_time) {
        strip_finished(state, config);
    }
}

void strip_thread(void* unused1, void* unused2, void* unused3) {

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    k_thread_cpu_pin(s_strip_thread, 0);

    {
        uint32_t animation_idx = 0;
        memset(&s_animations, 0x00, sizeof(s_animations));

        animation_create_static(&s_animations[animation_idx++]);
        animation_create_cycling(&s_animations[animation_idx++]);
        animation_create_wave(&s_animations[animation_idx++]);
    }

    const struct strip_config* const config = strip_get_config();
    struct strip_state* const state = strip_get_state_mut();

    uint32_t prev_time = 0;
    while (true) {

        const uint32_t start_time = k_uptime_get_32();
        const uint32_t dt_time = start_time - prev_time;
        prev_time = start_time;

        /* Reset pixels */

        strip_lock(K_FOREVER);

        memset(config->pixels, 0x00, config->pixels_count * sizeof(struct led_rgb));

        const bool enabled = config->enabled;
        if (enabled) {
            strip_update(dt_time, state, config);
        }

        strip_unlock();

        strip_sync();

        if (!enabled) {
            strip_wait_for_enabled(K_FOREVER);
        }

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

/******************************************************************************/
