
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

    result += color_lerp_comp((begin >> 16) & 0xFF, (end >> 16) & 0xFF, dt) << 16;
    result += color_lerp_comp((begin >> 8) & 0xFF, (end >> 8) & 0xFF, dt) << 8;
    result += color_lerp_comp((begin >> 0) & 0xFF, (end >> 0) & 0xFF, dt) << 0;

    return result;
}

static uint8_t color_intensity_comp(uint8_t c, uint8_t intensity)
{
    return (uint8_t)(((int)c * (int)intensity) / 100);
}

color_t color_intensity(color_t color, uint8_t intensity)
{
    uint32_t result = 0;

    intensity = MIN(100, intensity);

    result += color_intensity_comp((color >> 16) & 0xFF, intensity) << 16;
    result += color_intensity_comp((color >> 8) & 0xFF, intensity) << 8;
    result += color_intensity_comp((color >> 0) & 0xFF, intensity) << 0;

    return result;
}

/******************************************************************************/
