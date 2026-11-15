// Zigbee2MQTT external converter for the TS004F rotary knob (_TZ3000_gwkzibhs, ZG-101ZD)
// running the custom firmware with the rotary encoder entry (config "...;SA1u;ED4D7u;M;").
//
// The knob sends Level Control "Step (with On/Off)" on every detent and On/Off "Toggle" on a
// press. Bound to a lamp or a group it works without the coordinator; bound to the coordinator
// (done by this converter) the commands arrive here as actions:
//   brightness_step_up / brightness_step_down  with action_step_size, action_transition_time
//   toggle, brightness_move_up / brightness_move_down / brightness_stop (long press)
import * as m from "zigbee-herdsman-converters/lib/modernExtend";

export default [
    {
        zigbeeModel: ["TS004F-knob"],
        model: "TS004F-knob",
        vendor: "Custom devices (DiY)",
        description: "Rotary knob with custom firmware: one command per detent",
        // battery: firmware built with a battery pin (config entry BT<pin>)
        extend: [m.commandsOnOff(), m.commandsLevelCtrl(), m.battery({voltage: true})],
    },
];
