
#include "cmd.h"

#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

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
    color_t colors[STRIP_COLOR_MAX_COUNT];
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
                         JSON_TOK_NUMBER),
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

    LOG_INF("new cmd {%d, %d}", cmd.colors_count, cmd.intensity);

    for (size_t i = 0; i < cmd.colors_count; ++i) {
        strip_set_color(i, cmd.colors[i]);
    }

    strip_set_color_count(cmd.colors_count);

    strip_set_intensity(cmd.intensity);
    strip_set_animation(cmd.animation);

    return 0;
}

/******************************************************************************/
