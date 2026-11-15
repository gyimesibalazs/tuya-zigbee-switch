# TS004F rotary knob (`_TZ3000_gwkzibhs`, ZG-101ZD)

Battery powered Zigbee knob: a rotary encoder with a push button, one LED, one coin cell.
With this firmware it sends one Level Control command per detent, at once, so a lamp or a
group bound to it follows the hand ("smooth dimming"). The stock firmware collects the
rotation and sends it after a pause.

Status: works on one device. Built from the `knob-encoder` branch, not part of a release.

## Hardware

| | |
|---|---|
| MCU | Telink TLSR825x (chip id `0x5562`) |
| Flash | 1 MB, GigaDevice (JEDEC `c86014`) |
| Stock firmware | Tuya application on Telink Zigbee SDK 3.6.8.3, no bootloader, image at `0x0`, 146 kB |
| Stock Zigbee ids | manufacturer `_TZ3000_gwkzibhs`, model `TS004F` |
| IEEE address | at `0xFF000` (1 MB layout), kept when the firmware is replaced |

### Pins

Found with the chip stopped over SWS, all pins as inputs, watching which one changes
(`tlsr_c3.py pins`) and driving them one by one (`tlsr_c3.py led`).

| Function | Pin | Notes |
|---|---|---|
| Push button | `PA1` | low when pressed, needs the internal pull-up |
| Encoder A | `PD4` | goes low first when turned clockwise |
| Encoder B | `PD7` | goes low first when turned counter-clockwise |
| LED | `PC4` | lights when the pin is low (LED between the supply and the pin) |
| Battery measurement | `PB0` | any unconnected ADC pin works; `PB0`–`PB7` and `PC5` are all free |

The encoder lines have no pull-up resistors on the board: the internal ones are needed.
One detent is one full quadrature cycle: both lines rest high, go low one after the other
and return high.

### Programming pads

On the side of the board where the rotary encoder is, a row of three pads, from left to right:

| Pad | Signal |
|---|---|
| 1 | 3V (supply) |
| 2 | GND |
| 3 | SWS |

A reset pad was not looked for. It is not needed: the chip can be caught by switching its
supply on. With the ESP32-C3 based SWS flasher used here (tlsr-c3-flasher) the 3V pad goes
to the flasher's power pins, so every command power-cycles the target. With a plain USB-UART adapter connect SWS and 3V first and GND last, with the
flashing tool already waiting.

Remove the coin cell while the board is powered from the programmer.

## Building

```
make -C src/telink build \
    DEVICE_TYPE=end_device \
    CONFIG_STR="gwkzibhs;TS004F-knob;SA1u;ED4D7u;IC4i;BTB0;M;" \
    IMAGE_TYPE=47100 \
    VERSION_STR=1.1.3-knob5 FILE_VERSION=0x11033005 NVM_MIGRATIONS_VERSION=1 \
    BIN_FILE=../../out/knob.bin

python3 src/telink/make_ota.py create-ota --manufacturer-id 0x1141 --image-type 47100 \
    out/knob.bin out/knob.zigbee
```

Config string:

| Entry | Meaning |
|---|---|
| `SA1u` | switch (the push button) on `PA1`, pull-up |
| `ED4D7u` | rotary encoder, A on `PD4`, B on `PD7`, pull-ups (new entry, see below) |
| `IC4i` | indicator LED on `PC4`, inverted |
| `BTB0` | battery measurement through `PB0` |
| `M` | the switch is a momentary button |

## Flashing by wire

Back up the stock firmware first, then write, then erase what is left of the stock
firmware and its storage. The last sectors (`0xFE000`–`0xFFFFF`: calibration and IEEE
address) stay.

```
tlsr_c3.py -p <port> rf 0 0x100000 stock_knob.bin --twice
tlsr_c3.py -p <port> wf 0 knob.bin
tlsr_c3.py -p <port> es 0x26000 0x5A000
tlsr_c3.py -p <port> --run info
```

A later version is written with `wf 0 knob.bin` alone: the network data stays and the
knob comes back without pairing again.

The knob looks for a network by itself when it has none. Ten quick presses reset it to
factory new.

## Behaviour

- Every detent sends `Step (with On/Off)` (Level Control, command `0x06`) from endpoint 1 to
  the bindings, transition time 0.2 s. Clockwise is up.
- The step size follows the turning speed, measured as the time since the previous detent:

  | Time since the previous detent | Step (254 = full range) |
  |---|---|
  | 150 ms or more | 5 |
  | 90 – 150 ms | 10 |
  | 50 – 90 ms | 18 |
  | 25 – 50 ms | 30 |
  | less than 25 ms | 45 |

- Detents that arrive less than 40 ms after the last command are added up and go out with
  the next one, so none is lost and nothing waits for the knob to stop.
- Short press: `Toggle` (On/Off). Long press: `Move (with On/Off)`, release: `Stop`
  (the firmware's usual momentary switch behaviour).
- The LED blinks while the knob has no network and flashes on a press.
- Battery voltage and percentage on the Power Configuration cluster, 3.0 V = 100 %,
  2.0 V = 0 %, refreshed every 5 minutes.
- The knob stays awake for 3 seconds after the last detent, then goes to deep retention.
  Button and encoder lines wake it up.

### How the encoder is read

`src/base_components/encoder.c`. The GPIO callbacks run a few milliseconds after the edge
that raised the interrupt, and further edges are masked until then; on a fast turn both
lines have changed by that time. `hal_gpio_irq_levels()` returns the levels captured inside
the interrupt, right after the first edge: the line that is low there is the leading one.

After a wake-up from deep retention there is no such snapshot. Then the direction is taken
from what follows: seen both low, the line that returns high first is the leading one; seen
one line low and then rest, it was the trailing line of a detent that had already finished.

## Zigbee2MQTT

`zigbee2mqtt/converters/knob_custom.mjs` (external converter). It binds On/Off and Level
Control to the coordinator and exposes:

- `action`: `brightness_step_up`, `brightness_step_down` (with `action_step_size`,
  `action_transition_time`), `toggle`, `brightness_move_up`, `brightness_move_down`,
  `brightness_stop`
- `battery`, `voltage`
- `tx_power` (Basic cluster, attribute `0xff04`, int8, dBm; default 3)

The knob sleeps. A setting written from Zigbee2MQTT (transmit power, reconfigure,
reporting) only reaches it while it is awake: turn it while sending.

For a Zigbee lamp or group, bind endpoint 1 of the knob to it (On/Off and Level Control):
the commands then go directly, without the coordinator. For a lamp that is not on Zigbee
the actions have to be turned into service calls, for example in Home Assistant:

```yaml
triggers:
  - trigger: mqtt
    topic: zigbee2mqtt/<knob name>
conditions:
  - condition: template
    value_template: "{{ trigger.payload_json.action in ['brightness_step_up', 'brightness_step_down'] }}"
actions:
  - variables:
      before: "{{ state_attr('light.example', 'brightness') | int(0) }}"
      step: "{{ trigger.payload_json.action_step_size | int(8) }}"
      target: >
        {{ [before + step, 255] | min if trigger.payload_json.action == 'brightness_step_up'
           else [before - step, 3] | max }}
  - condition: template
    value_template: "{{ target != before }}"
  - action: light.turn_on
    target: {entity_id: light.example}
    data: {brightness: "{{ target }}", transition: 0.2}
  # wait for the lamp to report the new value, so that the next step starts from it
  - wait_template: "{{ state_attr('light.example', 'brightness') | int(0) != before }}"
    timeout: {milliseconds: 250}
    continue_on_timeout: true
mode: queued
max: 30
```

Without the wait, steps that arrive in quick succession all start from the same old
brightness and most of them are lost.

## OTA on the coin cell

Measured with firmware `knob3` (153 kB image):

| Poll intervals during the download | Result |
|---|---|
| short 500 ms, long 2 min (defaults), powered from the programmer | 47 % in 19 minutes, then stalled |
| short 250 ms, long 1 s, coin cell | 6 – 18 blocks per minute, many delivery failures, aborted by the knob at 2 % |
| short 500 ms, long 2 s, coin cell | steady 119 block requests per minute |

So on the coin cell a shorter poll interval made it worse, and the long poll interval has
to be short during the download, otherwise one lost answer stalls it for minutes. From
`knob5` on the firmware polls every 100 ms for the whole download by itself; how that
behaves on the coin cell is not measured yet.

To set the intervals from Zigbee2MQTT (values in quarter seconds; turn the knob so that it
is awake), publish to `zigbee2mqtt/<knob name>/1/set`:

```json
{"command": {"cluster": "genPollCtrl", "command": "setLongPollInterval", "payload": {"newLongPollInterval": 8}}}
{"command": {"cluster": "genPollCtrl", "command": "setShortPollInterval", "payload": {"newShortPollInterval": 2}}}
```

and put the long poll interval back to 480 (2 minutes) afterwards: the values are stored,
and a short long-poll interval drains the cell.

## Not done

- No `device_db.yaml` entry, so the board build, the generated converters and the OTA index
  do not know the knob yet.
- The current drawn asleep and while turning was not measured.
- Only one device was tested.
