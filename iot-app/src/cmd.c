
#include "cmd.h"

#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <stdlib.h>
#include <string.h>

#include "strip.h"

/******************************************************************************/

LOG_MODULE_REGISTER(cmd);

/******************************************************************************/

/*
{
  "colors":[1,2],
  "intensity":100,
  "enabled": true,
  "animation": 0
}
*/

struct led_cmd {
    bool enabled;
    uint32_t intensity;
    uint32_t animation;
    const char* colors[STRIP_COLOR_MAX_COUNT];
    uint32_t colors_count;
};

/******************************************************************************/

static const struct json_obj_descr s_led_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct led_cmd, enabled, JSON_TOK_TRUE),
    JSON_OBJ_DESCR_PRIM(struct led_cmd, intensity, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_ARRAY(struct led_cmd,
                         colors,
                         STRIP_COLOR_MAX_COUNT,
                         colors_count,
                         JSON_TOK_STRING),
};

static char s_json_buffer[2048];

/******************************************************************************/

int homethings_led_cmd(const uint8_t* data, size_t len) {

    if (!data || len == 0) {
        return -EINVAL;
    }

    const size_t json_len = MIN(sizeof(s_json_buffer), len);
    memcpy(&s_json_buffer[0], data, json_len);

    struct led_cmd cmd;
    int rc = 0;

    rc = json_obj_parse(
        &s_json_buffer[0], json_len, s_led_cmd_descr, ARRAY_SIZE(s_led_cmd_descr), &cmd);
    if (rc < 0) {
        LOG_WRN("cmd parse failed %d", rc);
        return rc;
    }

    printk("NEW STRIP CONFIG\n");
    printk("Intensity %u\n", cmd.intensity);

    for (size_t i = 0; i < cmd.colors_count; ++i) {
        color_t color = strtoul(cmd.colors[i], NULL, 16);
        color = sys_be32_to_cpu(color);
        color = (color & 0xFF00FF00) | (color & 0x000000FF) << 16 | (color & 0x00FF0000) >> 16;
        strip_set_color(i, color);
        printk("Color[%u]: #%08X Alpha:%u\n", i, color, color_get_alpha(color));
    }

    strip_set_color_count(cmd.colors_count);

    strip_set_intensity(cmd.intensity);
    strip_set_animation(cmd.animation);

    return 0;
}

/******************************************************************************/
