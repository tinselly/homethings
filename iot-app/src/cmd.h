
#ifndef HOMETHINGS_CMD_H_
#define HOMETHINGS_CMD_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

/******************************************************************************/

int homethings_led_cmd(const uint8_t* data, size_t len);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // HOMETHINGS_CMD_H_