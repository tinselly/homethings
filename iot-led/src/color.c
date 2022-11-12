
#include "color.h"

/******************************************************************************/

static color_comp_t color_lerp_comp(color_comp_t begin, color_comp_t end, uint8_t dt)
{
    // TODO: Change to integer calculation
    return begin + (color_comp_t)(((float)dt * (float)(end - begin)) / 100.0f);
}

color_t color_lerp(color_t begin, color_t end, uint8_t dt)
{
    color_t result = 0;

    dt = MIN(100, dt);

    result += color_lerp_comp((begin >> 24) & 0xFF, (end >> 24) & 0xFF, dt) << 24;
    result += color_lerp_comp((begin >> 16) & 0xFF, (end >> 16) & 0xFF, dt) << 16;
    result += color_lerp_comp((begin >> 8) & 0xFF, (end >> 8) & 0xFF, dt) << 8;
    result += color_lerp_comp((begin >> 0) & 0xFF, (end >> 0) & 0xFF, dt) << 0;

    return result;
}

static color_comp_t color_intensity_comp(color_comp_t c, color_comp_t intensity)
{
    return (color_comp_t)(((uint32_t)c * (uint32_t)intensity) / 255u);
}

color_t color_intensity(color_t color, color_comp_t intensity)
{
    uint32_t result = 0;

    result += color_intensity_comp((color >> 24) & 0xFF, intensity) << 24;
    result += color_intensity_comp((color >> 16) & 0xFF, intensity) << 16;
    result += color_intensity_comp((color >> 8) & 0xFF, intensity) << 8;
    result += color_intensity_comp((color >> 0) & 0xFF, intensity) << 0;

    return result;
}

color_t color_set_alpha(color_t color, color_comp_t alpha) {
    color_t result = 0;

    result += ((color_t)alpha) << 24;
    result += color & (0xFF << 16);
    result += color & (0xFF << 8);
    result += color & (0xFF << 0);

    return result;
}

color_comp_t color_get_alpha(color_t color) {
    return (color >> 24) & 0xFF;
}

/******************************************************************************/
