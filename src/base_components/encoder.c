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
        if (a != b) {
            encoder->pending = (a == 0) ? 1 : -1;
        } else if (irq_one_low) {
            encoder->pending = (irq_a == 0) ? 1 : -1;
        } else {
            // both already low and nothing better known: keep turning the way we were
            encoder->pending = encoder->last_direction != 0 ? encoder->last_direction : 1;
        }
        encoder->seen_both_low = (!a && !b);
        return;
    }

    if ((!a && !b) || (fresh && !irq_a && !irq_b)) {
        encoder->seen_both_low = 1;
    }
    if (a && b) {
        // Back at rest: a detent only if both lines were low on the way, otherwise the knob
        // was turned back before the detent (or one contact bounced).
        int8_t direction = encoder->pending;
        encoder->pending = 0;
        if (encoder->seen_both_low) {
            encoder_step(encoder, direction);
        }
        encoder->seen_both_low = 0;
    }
}

static void encoder_ignore(hal_gpio_pin_t pin, void *arg) {
    // the second line only has to raise the interrupt; encoder_update runs once per dispatch
}

void encoder_init(encoder_t *encoder) {
    encoder->pending        = 0;
    encoder->last_direction = 0;
    encoder->seen_both_low  = 0;
    hal_gpio_callback(encoder->pin_a, encoder_update, encoder);
    hal_gpio_callback(encoder->pin_b, encoder_ignore, encoder);
}
