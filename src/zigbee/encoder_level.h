#ifndef _ENCODER_LEVEL_H_
#define _ENCODER_LEVEL_H_

#include "switch_cluster.h"
#include <stdint.h>

/* Turns the detents of a rotary encoder into Level Control "Step (with On/Off)" commands sent
 * from the endpoint of a switch to its bindings. */
void encoder_level_init(zigbee_switch_cluster *cluster);

/* encoder_callback_t: param is ignored, direction +1 = brighter, -1 = darker */
void encoder_level_on_step(void *param, int8_t direction);

#endif
