# WiFi Controller (`wifi_ctrl`)

## Purpose

The `wifi_ctrl` module provides **station (STA) mode** Wi-Fi for the esp-platform: driver setup, scanning for access points, connecting with SSID/password, and forwarding link and IP events to the **manager** (`mgr_ctrl`) over the shared `msg_t` bus—similar to `eth_ctrl` for Ethernet.

It does **not** implement a captive portal or provisioning UI; those stay in higher layers (for example `lcd_ctrl`). This module exposes a small **message-driven API** and optional **C getters** for scan results.

---

## Build-time configuration (Kconfig)

Open menuconfig: **ESP32 - Platform → WiFi Controller**.

| Option | Meaning |
| ------ | ------- |
| **Enable WiFi Controller (STA)** (`CONFIG_WIFI_CTRL_ENABLE`) | Master switch. When disabled, the component is not linked and the `"wifi"` entry is omitted from `mgr_reg_list`. Default: **off**. |
| **Log level** (`CONFIG_WIFI_CTRL_LOG_LEVEL`) | Compile-time default verbosity for tag `ESP::WIFI`. |
| **Maximum AP records** (`CONFIG_WIFI_CTRL_SCAN_MAX_AP`) | How many `wifi_ap_record_t` entries are kept after a scan (1–40, default 20). Stronger APs tend to appear first; extra on-air APs are dropped. |

After changing options, rebuild the firmware (`idf.py build`).

> **Checked-in ESP32 defaults:** the repository's current `sdkconfig.defaults` enables `wifi_ctrl`, keeps Wi-Fi logs verbose for bring-up, and raises `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE` to `4096` so IP / SNTP / app handlers have more stack headroom.

### ESP-IDF Wi-Fi / PHY options

Ensure target Wi-Fi options in **Component config → Wi-Fi** (and PHY) match your board (country code, bandwidth, etc.). NVS must be initialized before the manager runs Wi-Fi init (the application already calls `NVS_Init()` in `app_main()` before `MGR_Init()`).

---

## Registration and startup order

When `CONFIG_WIFI_CTRL_ENABLE` is set:

1. The module is registered in `mgr_reg_list` as **`"wifi"`** with mask **`REG_WIFI_CTRL`**.
2. It is initialized **after** `eth` (if enabled) and **before** MQTT (which must remain last in the list).

Lifecycle (same pattern as other controllers):

| Phase | Called by MGR | Effect |
| ----- | ------------- | ------ |
| `WifiCtrl_Init` | During `MGR_Init` | `esp_netif_init` / default event loop (no-op if already done), default STA `esp_netif`, `esp_wifi_init`, event handlers, worker task and queue. |
| `WifiCtrl_Run` | During `MGR_Run` | `WIFI_MODE_STA`, `esp_wifi_start`. |
| `WifiCtrl_Done` | During `MGR_Done` | Stop STA, stop worker, unregister handlers, `esp_wifi_deinit`, destroy STA netif. |

You do not call these from application code unless you bypass the manager.

---

## Interaction with Ethernet

If **both** `eth_ctrl` and `wifi_ctrl` are enabled:

- Ethernet still initializes first and may call `esp_netif_init()` and `esp_event_loop_create_default()`; Wi-Fi tolerates **already-initialized** netif/event loop (`ESP_ERR_INVALID_STATE` is ignored where appropriate).
- STA uses **`esp_netif_create_default_wifi_sta()`**; Ethernet keeps its own netif. Both can coexist at the driver level.

**Manager behaviour (as implemented today in `mgr_ctrl.c`):**

- **`MSG_TYPE_ETH_IP`** — manager stores IP info, updates the string used for registration/MQTT topics, and calls **`mgr_StartMqtt()`**.
- **`MSG_TYPE_ETH_EVENT`** / **`DATA_ETH_EVENT_DISCONNECTED`** — calls **`mgr_StopMqtt()`**.
- **`MSG_TYPE_WIFI_IP`**, **`MSG_TYPE_WIFI_EVENT`**, and **`MSG_TYPE_WIFI_SCAN_RESULT`** — **not** handled in `mgr_ParseMsg()`; they hit the default branch (unknown type warning / error path). So **MQTT is not started on Wi-Fi “got IP”** and **Wi-Fi disconnect does not stop MQTT** until equivalent cases are added to the manager (mirror of Ethernet).
- Broadcast messages with **`to = REG_ALL_CTRL`** (e.g. **`MSG_TYPE_WIFI_EVENT`**, **`MSG_TYPE_WIFI_SCAN_RESULT`**) are still delivered to other modules (including **`lcd_ctrl`**) via **`mgr_NotifyCtrl()`** when those bits are set in `to`.

If you run **Ethernet and Wi-Fi at the same time**, you will need explicit policy in **`mgr_ctrl`** (which link owns MQTT, dual-IP handling, etc.); **`wifi_ctrl`** only emits events.

---

## Recommended flow: join a Wi-Fi network

### 1. Prerequisites

- Firmware built with **`CONFIG_WIFI_CTRL_ENABLE=y`**.
- Application has completed **`NVS_Init()`** then **`MGR_Init()` / `MGR_Run()`** so the Wi-Fi driver is started.

### 2. Discover networks (scan)

1. Send a **`MSG_TYPE_WIFI_SCAN_REQ`** message to **`REG_WIFI_CTRL`** via **`MGR_Send`** (or call **`WifiCtrl_Send`** from code running in the Wi-Fi module’s context—usually you route through the manager so `to` is correct).

   ```c
   msg_t m = {
     .type = MSG_TYPE_WIFI_SCAN_REQ,
     .from = REG_LCD_CTRL,   /* or your module */
     .to   = REG_WIFI_CTRL,
   };
   MGR_Send(&m);
   ```

2. Scan runs on an internal **worker task** (blocking `esp_wifi_scan_start`); do not call scan from the ESP event loop callback yourself.

3. On success, the module posts **`MSG_TYPE_WIFI_SCAN_RESULT`** with **`to = REG_ALL_CTRL`**. The payload holds:

   - `payload.wifi.u.scan.ap_count` — number of stored APs (≤ `CONFIG_WIFI_CTRL_SCAN_MAX_AP`).

4. Per-AP details are not read through extra `wifi_ctrl` functions; extend **`msg_t`** / add follow-up messages on the bus when your product needs SSID/RSSI rows after **`MSG_TYPE_WIFI_SCAN_RESULT`**.

5. If scan fails, you may receive **`MSG_TYPE_WIFI_EVENT`** with **`DATA_WIFI_EVENT_SCAN_FAILED`**.

### 3. Connect (SSID, password, parameters)

1. Fill **`data_wifi_connect_t`** inside **`msg.payload.wifi.u.connect`** and send **`MSG_TYPE_WIFI_CONNECT`** to **`REG_WIFI_CTRL`**:

   | Field | Usage |
   | ----- | ----- |
   | `ssid` | NUL-terminated string, max **32** characters (buffer is 33 bytes including `'\0'`). |
   | `password` | NUL-terminated; max **64** characters (65-byte buffer). Empty for open networks. |
   | `channel` | **`0`** = leave channel unspecified (normal case). Non-zero fixes the channel hint in `wifi_config_t.sta.channel`. |
   | `authmode` | Cast from **`wifi_auth_mode_t`** (see `esp_wifi_types.h`), e.g. `WIFI_AUTH_WPA2_PSK`, `WIFI_AUTH_OPEN`. Should match what you infer from scan (`wifi_ap_record_t.authmode`) or what you know about the AP. |

   ```c
   msg_t m = {
     .type = MSG_TYPE_WIFI_CONNECT,
     .from = REG_LCD_CTRL,
     .to   = REG_WIFI_CTRL,
   };
   strncpy(m.payload.wifi.u.connect.ssid, "MyNetwork", sizeof(m.payload.wifi.u.connect.ssid));
   m.payload.wifi.u.connect.ssid[sizeof(m.payload.wifi.u.connect.ssid) - 1] = '\0';
   strncpy(m.payload.wifi.u.connect.password, "secret", sizeof(m.payload.wifi.u.connect.password));
   m.payload.wifi.u.connect.password[sizeof(m.payload.wifi.u.connect.password) - 1] = '\0';
   m.payload.wifi.u.connect.channel  = 0;
   m.payload.wifi.u.connect.authmode = (uint8_t)WIFI_AUTH_WPA2_PSK;
   MGR_Send(&m);
   ```

2. The worker applies **`esp_wifi_set_config(WIFI_IF_STA, …)`** and **`esp_wifi_connect()`**.

3. Watch for:

   - **`MSG_TYPE_WIFI_EVENT`** / **`DATA_WIFI_EVENT_CONNECTED`** — association OK (no IP yet).
   - **`MSG_TYPE_WIFI_IP`** — STA received IPv4; same layout as Ethernet: `payload.wifi.u.ip_info` (`ip`, `mask`, `gw` as `uint32_t` in network byte order style as used elsewhere in the project).

4. To disconnect, send **`MSG_TYPE_WIFI_DISCONNECT`** to **`REG_WIFI_CTRL`**.

### 4. Optional: UI workflow (LCD / MQTT)

A typical product flow:

1. User opens “Wi-Fi” screen → firmware sends **`MSG_TYPE_WIFI_SCAN_REQ`**.
2. UI lists scan results from messages delivered to your module (e.g. after **`MSG_TYPE_WIFI_SCAN_RESULT`**, extend the payload or add follow-up messages as needed). Decode SSID as string; show lock icon from `authmode` where applicable.
3. User picks an AP → UI prompts for password if not open → send **`MSG_TYPE_WIFI_CONNECT`** with the right **`authmode`**.
4. On **`MSG_TYPE_WIFI_IP`**, show “connected” and allow cloud/MQTT usage.

Persisting credentials in **NVS** for the next boot is **not** implemented inside `wifi_ctrl`; add that in `cfg_ctrl`, `nvs_ctrl`, or a dedicated storage layer if required.

---

## Outbound messages (Wi-Fi → manager / others)

| Message | `to` | Payload / meaning |
| ------- | ---- | ----------------- |
| `MSG_TYPE_WIFI_EVENT` | `REG_ALL_CTRL` | `payload.wifi.u.event_id`: STA start/stop, connected, disconnected, scan failed. |
| `MSG_TYPE_WIFI_SCAN_RESULT` | `REG_ALL_CTRL` | `payload.wifi.u.scan.ap_count` — scan done; per-AP data is expected on the message bus (not via extra manager APIs). |
| `MSG_TYPE_WIFI_IP` | `REG_MGR_CTRL` | `payload.wifi.u.ip_info` — same shape as `MSG_TYPE_ETH_IP`. |

When `CONFIG_WIFI_CTRL_ENABLE` is on, the manager still treats **`MSG_TYPE_WIFI_SCAN_RESULT`** as an unknown type if addressed to **`REG_MGR_CTRL`**; prefer **`to`** masks that include the subscribers you need (e.g. **`REG_ALL_CTRL`** for broadcast), and handle payloads in each module’s **`send_fn`** (e.g. **`LcdCtrl_Send`** for **`MSG_TYPE_WIFI_EVENT`**). There is no separate Wi-Fi list API in the manager; modules use **`mgr_reg_t`** and **`msg_t`** as usual.

To get **LCD IP text** from Wi-Fi, either extend **`to`** on **`MSG_TYPE_WIFI_IP`** so **`REG_LCD_CTRL`** receives a copy, or have the manager forward IP after it handles Wi-Fi IP (once implemented).

---

## Inbound commands (others → Wi-Fi)

Handled only in **`WifiCtrl_Send`**:

| `msg.type` | Action |
| ---------- | ------ |
| `MSG_TYPE_WIFI_SCAN_REQ` | Queue scan on worker task. |
| `MSG_TYPE_WIFI_CONNECT` | Apply STA config and connect. |
| `MSG_TYPE_WIFI_DISCONNECT` | `esp_wifi_disconnect()`. |

Other message types are ignored (returns `ESP_OK`).

Use **`to = REG_WIFI_CTRL`** so the manager forwards the message only to the Wi-Fi controller. Avoid broadcasting **`REG_ALL_CTRL`** for commands you do not want every module to see.

---

## Public API summary

Declared in `modules/wifi_ctrl/include/wifi_ctrl.h`:

- **`WifiCtrl_Init` / `WifiCtrl_Done` / `WifiCtrl_Run`** — invoked by the manager; do not duplicate from app code under normal use.
- **`WifiCtrl_Send(const msg_t *msg)`** — enqueue scan / connect / disconnect (also used internally for shutdown).

---

## Troubleshooting

| Symptom | Things to check |
| ------- | ---------------- |
| Build does not include Wi-Fi | `CONFIG_WIFI_CTRL_ENABLE=y`, rebuild; `main` must `PRIV_REQUIRES wifi_ctrl` (already conditional in `main/CMakeLists.txt`). |
| Scan always empty / fails | Antenna, RF calibration, country code, 2.4 GHz only on many ESP32 boards; log level `Debug` on `ESP::WIFI`. |
| Association fails | Wrong password, wrong **`authmode`**, hidden SSID (may need extra scan config—see ESP-IDF docs), or WPA3-only AP with STA profile mismatch. |
| No IP | DHCP on AP; check `IP_EVENT_STA_GOT_IP` path and router logs. |
| MQTT never starts on Wi-Fi-only builds | **`mgr_ctrl`** must handle **`MSG_TYPE_WIFI_IP`** like **`MSG_TYPE_ETH_IP`** (not implemented yet). |
| MQTT stays up after Wi-Fi drops | **`MSG_TYPE_WIFI_EVENT`** / disconnect is not wired to **`mgr_StopMqtt()`** yet; same fix as above. |
| Log spam / “Unknown message” for Wi-Fi | Wi-Fi messages sent **only** to **`REG_MGR_CTRL`** are unrecognized by **`mgr_ParseMsg`** until cases are added. |

---

## Further reading (Espressif)

- **Wi-Fi Driver → Scan** — `esp_wifi_scan_start`, `esp_wifi_scan_get_ap_records`.
- **Wi-Fi → Station General Scenario** — `wifi_config_t`, `esp_wifi_set_config`, `esp_wifi_connect`.
- **ESP-NETIF** — default STA creation and `IP_EVENT_STA_GOT_IP`.

These topics are in the ESP-IDF Programming Guide for your IDF version (e.g. 5.x).
