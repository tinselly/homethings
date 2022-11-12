
#ifndef HOMETHINGS_STRIP_H_
#define HOMETHINGS_STRIP_H_

#include "color.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define STRIP_COLOR_MAX_COUNT 6

/******************************************************************************/

struct strip_config {
    color_t colors[STRIP_COLOR_MAX_COUNT];
    size_t colors_count;
    struct led_rgb* pixels;
    size_t pixels_count;
    uint8_t intensity;
    uint32_t animation_time;
    uint32_t animation;
    bool enabled;
};

struct strip_state {
    uint32_t time;
    uint32_t animation_dt;
    size_t color_prev_idx;
    size_t color_next_idx;
};

/******************************************************************************/

void strip_color_to_rgb(color_t color, struct led_rgb* rgb);

int strip_lock(k_timeout_t timeout);
void strip_unlock();
int strip_wait_for_enabled(k_timeout_t timeout);

const struct strip_config* strip_get_config();
struct strip_config* strip_get_config_mut();

const struct strip_state* strip_get_state();
struct strip_state* strip_get_state_mut();

int strip_sync();

void strip_set_color_count(size_t count);
void strip_set_color(size_t i, color_t color);
void strip_set_intensity(uint8_t intensity);
void strip_set_animation(uint8_t animation);
void strip_set_enabled(bool enabled);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_STRIP_H_