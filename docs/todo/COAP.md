# CoAP Controller Module (`coap_ctrl`)

Direct peer-to-peer communication between **ESP32-EVB** (controller) and **ESP32-S3-ETH** (sensor node) over CoAP/UDP — without an external MQTT broker.

---

## Architecture

Both nodes run the same CoAP stack. Each is simultaneously a **server** (exposes its own resources) and a **client** (queries the peer). Communication is bidirectional over standard UDP port 5683.

```
ESP32-S3-ETH  (Sensor Node)          ESP32-EVB  (Controller Node)
─────────────────────────────         ─────────────────────────────
SERVER                                SERVER
  GET /sensor/lux    (Observable)       GET /control/heater
  GET /sensor/temp   (Observable)       PUT /control/heater
                                        GET /control/pump
CLIENT                                  PUT /control/pump
  PUT /control/heater  ──────────►      GET /status
  PUT /control/pump
                                      CLIENT
               ◄────────────────────    GET /sensor/lux + Observe
                    Observe push
```

The **Observe** mechanism (RFC 7641) replaces MQTT subscribe: the controller subscribes once with a single GET, then receives automatic push notifications every time the sensor reads a new lux value — no polling required.

---

## File Structure

```
modules/coap_ctrl/
├── CMakeLists.txt          — conditional compilation per node role
├── Kconfig.inc             — menuconfig options: role, peer IP, port, lux thresholds
├── coap_ctrl.c             — lifecycle (Init / Done / Run / Send) + FreeRTOS task
├── coap_sensor.c           — sensor node: server resources + client PUT helpers
├── coap_controller.c       — controller node: server resources + Observe client
└── include/
    ├── coap_ctrl.h         — public API (CoapCtrl_*, coap_ctrl_update_lux)
    ├── coap_defs.h         — shared resource paths, payload_coap_t, message subtypes
    ├── coap_sensor.h       — internal sensor node API
    └── coap_controller.h   — internal controller node API
```

### Role selection (Kconfig)

The same source tree builds two different firmware images. The node role is selected in `idf.py menuconfig` under **CoAP Controller → Node role**:

| Option | Target board | Compiled files |
|---|---|---|
| `COAP_CTRL_SENSOR_NODE` | ESP32-S3-ETH | `coap_ctrl.c` + `coap_sensor.c` |
| `COAP_CTRL_CONTROLLER_NODE` | ESP32-EVB | `coap_ctrl.c` + `coap_controller.c` |

### Integration with existing platform files

Four small changes are needed in the existing project (described in the `PATCH_*.txt` files):

| File | Change |
|---|---|
| `include/msg.h` | Add `#define REG_COAP_CTRL (1 << 19)` |
| `include/mgr_reg_list.h` | Add `#include "coap_ctrl.h"` and a registry entry |
| `modules/Kconfig.inc` | Add `orsource "coap_ctrl/Kconfig.inc"` |
| `main/CMakeLists.txt` | Add `if(CONFIG_COAP_CTRL_ENABLE)` block |

---

## Data Flow

### Sensor node → Controller (Observe push)

```
sensor_tsl2561.c (TSL2561 task)
  │
  │  tsl2561_GetLux() every 1 s
  │
  ▼
coap_ctrl_update_lux(lux)
  │  posts COAP_MSG_UPDATE_LUX to coap task queue
  ▼
coapctrl_TaskFn()  [FreeRTOS task]
  │  drains queue, calls coap_sensor_notify_lux()
  ▼
coap_sensor_notify_lux(lux)
  │  stores new value in s_lux
  │  coap_resource_notify_observers(s_res_lux)
  ▼
libcoap I/O loop
  │  sends CON response with {"lux": <value>}
  │  to all registered Observers
  ▼
ESP32-EVB  ←──────────────────────── UDP push
  │
  ▼
coap_controller_on_response()
  │  parses {"lux": <value>}
  │  applies threshold logic
  ▼
set_heater(1 or 0)
  └─ TODO: gpio_set_level(GPIO_NUM_32, state)
         / relay_ctrl_set(0, state)
```

### Controller → Sensor node (PUT command)

```
coap_controller_on_response()   [or any other trigger]
  │
  ▼  [optional — sensor-side decision]
coap_sensor_put_heater(ctx, state)
  │  builds CON PUT /control/heater {"state":1}
  ▼
libcoap → UDP → ESP32-EVB
  │
  ▼
handler_put_heater()
  │  parses {"state":1}
  ▼
set_heater(state)
  └─ gpio / relay
```

### Task and I/O loop

The module runs a single FreeRTOS task (`coap-task`, stack 8 KB, priority 9). The task alternates between two operations without blocking either:

```
loop:
  coap_io_process(ctx, 200 ms)   ← drives retransmissions, ACKs, Observe heartbeats
  while xQueueReceive(..., 0):   ← drains inter-module messages (non-blocking)
    coapctrl_ParseMsg()
```

This is the same pattern used by `sys_ctrl.c` with `sysctrl_GetQueueWaitTicks()` — the I/O timeout acts as the scheduler tick for the CoAP engine.

---

## Key Points

### Module follows the platform pattern exactly

`coap_ctrl` implements the same four-function public API as every other module (`Init`, `Done`, `Run`, `Send`) and registers in `mgr_reg_list[]` with its own `REG_COAP_CTRL` bit. The manager starts, stops and routes messages to it identically to `relay_ctrl`, `sensor_ctrl` or any other module.

### Bidirectional communication

Both directions are fully supported:

- **Sensor → Controller**: via CoAP **Observe** push — the controller subscribes once after boot and receives automatic notifications on every measurement, exactly like MQTT subscribe.
- **Controller → Sensor** (or vice versa): via CoAP **PUT** — the node that detects a threshold crossing sends a PUT request to change the relay state on the peer.

### Where the relay decision lives

The threshold logic can run on either node. The default implementation puts it on the **controller** (`coap_controller_on_response()`):

```
lux > LUX_THRESHOLD_OFF  →  set_heater(1)
lux < LUX_THRESHOLD_ON   →  set_heater(0)
```

An alternative commented-out block in `coap_sensor_notify_lux()` moves the decision to the **sensor node**, which then sends a PUT to the controller. Only one side should be active at a time.

### Migration without downtime

`coap_ctrl_update_lux()` can be called from `sensor_tsl2561.c` in parallel with the existing `tsl2561_cb()` MQTT path. Both channels are independent and can run simultaneously, which allows gradual migration:

1. Enable `CONFIG_COAP_CTRL_ENABLE` on both nodes.
2. Verify CoAP communication works alongside MQTT.
3. Disable `CONFIG_MQTT_CTRL_ENABLE` once confident.

### Kconfig options summary

| Option | Default | Description |
|---|---|---|
| `COAP_CTRL_ENABLE` | `n` | Enable the module |
| `COAP_CTRL_NODE_ROLE` | `SENSOR_NODE` | Select compiled role |
| `COAP_CTRL_PEER_IP` | `192.168.1.100` | IP of the remote node |
| `COAP_CTRL_PORT` | `5683` | Standard CoAP UDP port |
| `COAP_CTRL_OBSERVE_INTERVAL_MS` | `5000` | Push interval [ms] |
| `COAP_CTRL_LUX_THRESHOLD_ON` | `200` | Lux level to turn heater ON |
| `COAP_CTRL_LUX_THRESHOLD_OFF` | `500` | Lux level to turn heater OFF |

### TODO stubs for hardware integration

Two places need filling in with project-specific calls:

- `coap_controller.c` → `set_heater()` and `set_pump()`: replace comments with `gpio_set_level()` or `relay_ctrl_set()` calls matching the ESP32-EVB relay GPIO assignments (GPIO 32 and 33, as used in `relay_ctrl.c`).
- `sensor_tsl2561.c`: add `coap_ctrl_update_lux(tsl2561_lux)` next to the existing MQTT callback.
