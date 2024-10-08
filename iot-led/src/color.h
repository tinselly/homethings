
#ifndef HOMETHINGS_COLOR_H_
#define HOMETHINGS_COLOR_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

typedef uint32_t color_t;     // RGB Color
typedef uint8_t color_comp_t; // Component of the RGB color

/******************************************************************************/

/**
 * @brief Linear interpolation between begin and end colors
 *
 * @param lhs Begin Color
 * @param rhs End Color
 * @param dt  Delta in range 0 - 100 %
 * @return Result color
 */
color_t color_lerp(color_t begin, color_t end, uint8_t dt);

/**
 * @brief
 *
 * @param color
 * @param intensity Intensity in range 0 - 255
 * @return Result color
 */
color_t color_intensity(color_t color, color_comp_t intensity);

color_t color_set_alpha(color_t color, color_comp_t intensity);

color_comp_t color_get_alpha(color_t color);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_COLOR_H_