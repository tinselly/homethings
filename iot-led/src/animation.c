
#include "animation.h"

#include <zephyr/drivers/led_strip.h>

#include <string.h>

/******************************************************************************/

void animation_fill_color(const struct strip_config* config, color_t color) {
    struct led_rgb rgb;

    strip_color_to_rgb(color, &rgb);

    for (size_t i = 0; i < config->pixels_count; ++i) {
        memcpy(&config->pixels[i], &rgb, sizeof(struct led_rgb));
    }
}

void animation_set_color(size_t i, const struct strip_config* config, color_t color) {
    config->pixels[i].r = (color >> 16) & 0xFF;
    config->pixels[i].g = (color >> 8) & 0xFF;
    config->pixels[i].b = (color >> 0) & 0xFF;
}

/******************************************************************************/

static void
animate_static(const struct strip_state* state, const struct strip_config* config, void* ctx) {

    color_t result = config->colors[0];

    result = color_intensity(result, color_get_alpha(result));
    result = color_intensity(result, config->intensity);

    animation_fill_color(config, result);
}

void animation_create_static(struct animation* animation) {
    animation->animate = &animate_static;
    animation->ctx = NULL;
}

/******************************************************************************/

static void
animate_cycling(const struct strip_state* state, const struct strip_config* config, void* ctx) {

    color_t result = color_lerp(config->colors[state->color_prev_idx],
                                config->colors[state->color_next_idx],
                                state->animation_dt);

    result = color_intensity(result, color_get_alpha(result));
    result = color_intensity(result, config->intensity);

    animation_fill_color(config, result);
}

void animation_create_cycling(struct animation* animation) {
    animation->animate = &animate_cycling;
    animation->ctx = NULL;
}

/******************************************************************************/

static void
animate_wave(const struct strip_state* state, const struct strip_config* config, void* ctx) {}

void animation_create_wave(struct animation* animation) {
    animation->animate = &animate_wave;
    animation->ctx = NULL;
}

/******************************************************************************/
