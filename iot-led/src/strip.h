
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
    size_t pixels_count;
    uint8_t intensity;
    uint32_t animation_time;
    uint32_t animation;
};

struct strip_state {
    const struct strip_config* config;
    uint32_t time;
    size_t color_prev;
    size_t color_next;
};

typedef void(*strip_animate_t)(const struct strip_state* state);

/******************************************************************************/

void strip_set_color_count(size_t count);
void strip_set_color(size_t i, color_t color);
void strip_set_intensity(uint8_t intensity);
void strip_set_animation(uint8_t animation);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_STRIP_H_