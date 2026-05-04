# Build and flash (ESP-IDF 5.5.x)

This project currently targets **`esp32`**, **`esp32s2`**, and **`esp32s3`** boards. Use the board-specific defaults files in the repository root before switching targets.

## 1. Prepare the ESP-IDF shell

Source the ESP-IDF environment in every fresh shell session:

```bash
. $IDF_PATH/export.sh
idf.py --version
idf.py --list-targets
```

## 2. Pick the matching defaults file

Copy the board-specific defaults you want to use into `sdkconfig.defaults` **before** running `idf.py set-target ...`:

| IDF target | Typical board in this repo | Defaults file | Typical Linux serial port |
| ---------- | -------------------------- | ------------- | ------------------------- |
| `esp32` | Olimex ESP32-EVB | `sdkconfig.defaults.esp32.debug` | `/dev/ttyUSB0` |
| `esp32s2` | Wemos / LOLIN S2 Mini | `sdkconfig.defaults.esp32s2.debug` | `/dev/ttyACM0` |
| `esp32s3` | Waveshare ESP32-S3-ETH | `sdkconfig.defaults.esp32s3.debug` | `/dev/ttyACM0` |

Example:

```bash
cp sdkconfig.defaults.esp32.debug sdkconfig.defaults
```

> `idf.py set-target ...` recreates `sdkconfig`. If you want to preserve your current configuration first, run `idf.py save-defconfig`.

## 3. Set the target

```bash
idf.py set-target esp32
# or: esp32s2 / esp32s3
```

## 4. Configure and build

```bash
idf.py menuconfig
idf.py build
```

Current checked-in ESP32 defaults (`sdkconfig.defaults`) include:

- **4 MB** flash size
- custom partition table: `config/partitions-esp32.csv`
- enabled Wi-Fi, MQTT, LCD, relay, system, sensor, and CLI controllers
- `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096` for better headroom when IP / SNTP / app handlers run together

If your board uses a different flash size, partition layout, or network wiring, adjust those options in `menuconfig` before building.

## 5. Flash and monitor

### ESP32 / Olimex ESP32-EVB

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### ESP32-S2 / ESP32-S3 boards with native USB

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

If you are unsure which serial device to use, check:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

## 6. Useful maintenance commands

```bash
idf.py size            # image / section size summary
idf.py save-defconfig  # save current config back to sdkconfig.defaults
idf.py fullclean       # remove build output and reconfigure from scratch
```

## Notes

- `sdkconfig` is the generated working configuration for the current build tree; it may be overwritten by `set-target`, `menuconfig`, or a clean reconfigure.
- Prefer committing updates to `sdkconfig.defaults*` rather than hand-editing `sdkconfig`.
- When `cli_ctrl` is enabled, the serial monitor exposes the project REPL prompt as `esp>` after boot.
