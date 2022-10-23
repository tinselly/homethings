
#ifndef HOMETHINGS_STRIP_H_
#define HOMETHINGS_STRIP_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define STRIP_COLOR_PALETTE_SIZE 6

/******************************************************************************/

void strip_set_color_palette(size_t i, uint32_t color);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_STRIP_H_