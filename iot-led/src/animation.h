
#ifndef HOMETHINGS_ANIMATION_H_
#define HOMETHINGS_ANIMATION_H_

#include "color.h"
#include "strip.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

/******************************************************************************/

typedef void (*animate_func_t)(const struct strip_state* state,
                               const struct strip_config* config,
                               void* ctx);

struct animation {
    animate_func_t animate;
    void* ctx;
};

/******************************************************************************/

void animation_create_static(struct animation* animation);
void animation_create_cycling(struct animation* animation);
void animation_create_wave(struct animation* animation);

/******************************************************************************/

void animation_fill_color(const struct strip_config* config, color_t color);
void animation_set_color(size_t i, const struct strip_config* config, color_t color);
size_t animation_next_pixel_index(size_t i, const struct strip_config* config);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_ANIMATION_H_