#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "hal/gpio.h"
#include <stdint.h>

/* Rotary encoder with two contacts (quadrature) and one full cycle per detent:
 * both lines rest high, the line that goes low first tells the direction. */

typedef void (*encoder_callback_t)(void *param, int8_t direction);

typedef struct {
    hal_gpio_pin_t     pin_a;          // falls first when turned in the positive direction
    hal_gpio_pin_t     pin_b;
    encoder_callback_t on_step;        // called once per detent, direction +1 or -1
    void *             callback_param;
    int8_t             pending;        // direction of the cycle in progress, 0 = at rest
    int8_t             last_direction;
    uint8_t            seen_both_low;
} encoder_t;

void encoder_init(encoder_t *encoder);

#endif
