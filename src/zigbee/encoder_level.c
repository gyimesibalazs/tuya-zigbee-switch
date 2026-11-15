#include "encoder_level.h"
#include "consts.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include "zigbee_commands.h"

/*
 * Every detent is sent at once. Only when detents come faster than the radio can send them
 * (ENCODER_MIN_SEND_INTERVAL_MS) the ones in between are added up and go out together in the
 * next command, so nothing is lost and nothing waits for the knob to stop.
 */
/* Level units per detent (254 = full range), chosen by the time since the previous detent:
 * slow turning gives fine steps, a quick flick crosses the whole range. */
typedef struct {
    uint16_t max_interval_ms; // detents closer than this ...
    uint8_t  step;            // ... move the level by this much
} encoder_speed_t;

static const encoder_speed_t encoder_speeds[] = {
    {  25, 45 },
    {  50, 30 },
    {  90, 18 },
    { 150, 10 },
};
#ifndef ENCODER_SLOW_STEP_SIZE
#define ENCODER_SLOW_STEP_SIZE           5
#endif
#ifndef ENCODER_STAY_AWAKE_MS
#define ENCODER_STAY_AWAKE_MS            3000 // no sleep while the knob is being turned
#endif
#ifndef ENCODER_MIN_SEND_INTERVAL_MS
#define ENCODER_MIN_SEND_INTERVAL_MS     40
#endif
#ifndef ENCODER_TRANSITION_TIME
#define ENCODER_TRANSITION_TIME          2   // 1/10 s: the lamp fades over this time, which hides the steps
#endif

static zigbee_switch_cluster *level_cluster = NULL;
static hal_task_t             flush_task;
static int16_t                pending_units = 0;   // signed level units not sent yet
static uint32_t               last_step_ms  = 0;
static uint32_t               last_send_ms  = 0;
static uint8_t                sent_once     = 0;
static uint8_t                stepped_once  = 0;

static void encoder_level_flush(void *arg) {
    if (pending_units == 0 || level_cluster == NULL) {
        return;
    }
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        pending_units = 0;
        return;
    }

    uint16_t size = pending_units > 0 ? pending_units : -pending_units;
    if (size > 254) {
        size = 254;
    }

    static uint8_t payload[4];
    payload[0] = pending_units > 0 ? ZCL_LEVEL_MOVE_UP : ZCL_LEVEL_MOVE_DOWN; // step mode: 0 up, 1 down
    payload[1] = (uint8_t)size;
    payload[2] = ENCODER_TRANSITION_TIME & 0xff;
    payload[3] = ENCODER_TRANSITION_TIME >> 8;
    pending_units = 0;

    hal_zigbee_cmd c = {
        .endpoint            = level_cluster->endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_LEVEL_CONTROL,
        .command_id          = ZCL_CMD_LEVEL_STEP_WITH_ON_OFF,
        .cluster_specific    = 1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp = 1,
        .manufacturer_code   = 0,
        .payload             = payload,
        .payload_len         = sizeof(payload),
    };
    hal_zigbee_send_cmd_to_bindings(&c);
    last_send_ms = hal_millis();
    sent_once    = 1;
}

void encoder_level_init(zigbee_switch_cluster *cluster) {
    level_cluster      = cluster;
    flush_task.handler = encoder_level_flush;
    flush_task.arg     = NULL;
    hal_tasks_init(&flush_task);
}

void encoder_level_on_step(void *param, int8_t direction) {
    uint32_t now      = hal_millis();
    uint32_t interval = now - last_step_ms;
    uint8_t  step     = ENCODER_SLOW_STEP_SIZE;

    if (stepped_once) {
        for (uint8_t i = 0; i < sizeof(encoder_speeds) / sizeof(encoder_speeds[0]); i++) {
            if (interval < encoder_speeds[i].max_interval_ms) {
                step = encoder_speeds[i].step;
                break;
            }
        }
    }
    last_step_ms = now;
    stepped_once = 1;
    hal_system_stay_awake(ENCODER_STAY_AWAKE_MS);

    // a change of direction drops what was still waiting for the other direction
    if ((direction > 0 && pending_units < 0) || (direction < 0 && pending_units > 0)) {
        pending_units = 0;
    }
    pending_units += direction * step;

    uint32_t since_send = now - last_send_ms;
    if (!sent_once || since_send >= ENCODER_MIN_SEND_INTERVAL_MS) {
        hal_tasks_unschedule(&flush_task);
        encoder_level_flush(NULL);
    } else {
        hal_tasks_schedule(&flush_task, ENCODER_MIN_SEND_INTERVAL_MS - since_send);
    }
}
