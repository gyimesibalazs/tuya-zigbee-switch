#include "hal/system.h"
#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#pragma pack(pop)
#include <stdbool.h>
#include <stdint.h>

void hal_system_reset(void) {
    // Telink 8258 system reset
    mcu_reset();
}

void hal_factory_reset(void) {
    zb_factoryReset();
}

#include "hal/timer.h"

static uint32_t stay_awake_until = 0;
static bool     stay_awake_set   = false;

void hal_system_stay_awake(uint32_t ms) {
    stay_awake_until = hal_millis() + ms;
    stay_awake_set   = true;
}

// for the main loop: true while sleeping is not wanted
bool telink_stay_awake(void) {
    if (!stay_awake_set) {
        return false;
    }
    if ((int32_t)(stay_awake_until - hal_millis()) > 0) {
        return true;
    }
    stay_awake_set = false;
    return false;
}
