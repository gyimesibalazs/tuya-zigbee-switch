#include "encoder.h"
#include <stddef.h>

/*
 * The GPIO callbacks arrive a few milliseconds after the edge that caused the interrupt, and
 * further edges are masked until then. On a fast turn the two lines change within that time, so
 * the levels read here are not enough to tell the direction. hal_gpio_irq_levels() gives the
 * levels as they were inside the interrupt, right after the first edge.
 */
static void encoder_step(encoder_t *encoder, int8_t direction) {
    encoder->last_direction = direction;
    if (encoder->on_step != NULL) {
        encoder->on_step(encoder->callback_param, direction);
    }
}

#define ENCODER_DIRECTION_UNKNOWN    2

static void encoder_update(hal_gpio_pin_t pin, void *arg) {
    encoder_t *encoder = (encoder_t *)arg;
    uint8_t    a       = hal_gpio_read(encoder->pin_a);
    uint8_t    b       = hal_gpio_read(encoder->pin_b);
    uint8_t    irq_a   = 1, irq_b = 1;
    uint8_t    fresh   = hal_gpio_irq_levels(encoder->pin_a, encoder->pin_b, &irq_a, &irq_b);
    uint8_t    irq_one_low = fresh && (irq_a != irq_b);

    if (encoder->pending == 0) {
        if (a && b) {
            // At rest again. If exactly one line was low at the first edge, a whole detent
            // passed while the interrupt was masked.
            if (irq_one_low) {
                encoder_step(encoder, irq_a == 0 ? 1 : -1);
            }
            return;
        }
        encoder->uncertain     = 0;
        encoder->seen_both_low = (!a && !b);
        if (irq_one_low) {
            // the line that was low right after the first edge is the leading one; the current
            // levels may already show the trailing line alone, which would look like the
            // opposite direction
            encoder->pending = (irq_a == 0) ? 1 : -1;
        } else if (a != b) {
            // No interrupt levels: the device was asleep and the edge only woke it up. The line
            // that is low alone is the leading one, unless the wake-up took so long that the
            // leading line is already back high. Decided by what comes next.
            encoder->pending   = (a == 0) ? 1 : -1;
            encoder->uncertain = !fresh;
        } else {
            // both already low: the line that goes back high first is the leading one
            encoder->pending = ENCODER_DIRECTION_UNKNOWN;
        }
        return;
    }

    if ((!a && !b) || (fresh && !irq_a && !irq_b)) {
        encoder->seen_both_low = 1;
    }
    if (encoder->pending == ENCODER_DIRECTION_UNKNOWN && a != b) {
        encoder->pending = a ? 1 : -1;
    }
    if (a && b) {
        int8_t direction = encoder->pending;
        encoder->pending = 0;
        if (direction == ENCODER_DIRECTION_UNKNOWN) {
            // never saw the lines apart: keep turning the way we were
            encoder_step(encoder, encoder->last_direction != 0 ? encoder->last_direction : 1);
        } else if (encoder->seen_both_low) {
            encoder_step(encoder, direction);
        } else if (encoder->uncertain) {
            // woke up late: what we saw low alone was the trailing line of a finished detent
            encoder_step(encoder, -direction);
        }
        // otherwise the knob was turned back before the detent, or one contact bounced
        encoder->seen_both_low = 0;
        encoder->uncertain     = 0;
    }
}

static void encoder_ignore(hal_gpio_pin_t pin, void *arg) {
    // the second line only has to raise the interrupt; encoder_update runs once per dispatch
}

void encoder_init(encoder_t *encoder) {
    encoder->pending        = 0;
    encoder->last_direction = 0;
    encoder->seen_both_low  = 0;
    encoder->uncertain      = 0;
    hal_gpio_callback(encoder->pin_a, encoder_update, encoder);
    hal_gpio_callback(encoder->pin_b, encoder_ignore, encoder);
}
