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
        ota: true,
        // battery: firmware built with a battery pin (config entry BT<pin>)
        extend: [
            m.commandsOnOff(),
            m.commandsLevelCtrl(),
            m.battery({voltage: true}),
            // Basic cluster, custom attribute 0xff04 (int8): radio transmit power.
            // The knob sleeps: turn it while changing the value, so that it is awake to take it.
            m.numeric({
                name: "tx_power",
                unit: "dBm",
                cluster: "genBasic",
                attribute: {ID: 0xff04, type: 0x28},
                valueMin: -25,
                valueMax: 10,
                valueStep: 1,
                description: "Radio transmit power (default 3 dBm). Turn the knob while changing it.",
                entityCategory: "config",
            }),
        ],
    },
];
