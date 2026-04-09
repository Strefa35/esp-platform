# Idea

The idea of the project was created to control the switching on of an electric water heater based on the intensity of sunlight. This was to enable the use of the surplus energy produced by the solar panels to heat utility water in a single-family house. 

In order to fully automate this process, it was necessary to build a controller that would turn the power supply of the electric water heater on or off when the solar panels produce the most energy. Its excess is discharged into the power company's network.

## Assumptions
- simple and cheap construction
- easy configuration, also remote
- easy expansion
- possibility of connecting various sensors (temperature, humidity, light intensity, etc.)

# ESP Platform
That's why I created my own software platform to easily configure modules depending on the ESP32 board version.

Currently, the following boards are supported:
- ESP32
  - [ESP32-EVB](https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)
  - [Hardware reference](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/hw-reference/index.html)
- ESP32-S3
  - [ESP32-S3-ETH](https://www.waveshare.com/wiki/ESP32-S3-ETH)
  - [Hardware reference](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/hw-reference/index.html)
- ESP32-S2
  - [ESP32-S2 Mini](https://www.wemos.cc/en/latest/s2/s2_mini.html)
  - [Hardware reference](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s2/hw-reference/index.html)

The above boards have their configurations available in sdkconfig.defaults files:
- sdkconfig.defaults.esp32.debug
- sdkconfig.defaults.esp32s3.debug
- sdkconfig.defaults.esp32s2.debug

### Partition table (ESP32)

The ESP32 build uses a **custom partition table** from [`config/partitions-esp32.csv`](config/partitions-esp32.csv). It is selected via `CONFIG_PARTITION_TABLE_CUSTOM=y` and `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="config/partitions-esp32.csv"` in `sdkconfig.defaults` (and `sdkconfig` after configuration). If you rename the CSV file or add per-target tables, update those Kconfig options or set **Partition Table → Custom partition CSV file** in `idf.py menuconfig`.

Set **Serial flasher config → Flash size** to match your module (e.g. **4 MB** on [ESP32-EVB](https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)). The partition layout must fit within that size.

## List of supported targets
`idf.py --list-targets`

## Change target

### Set ESP32 target
```
cp sdkconfig.defaults.esp32.debug sdkconfig.defaults
idf.py set-target esp32
idf.py menuconfig
```
### Set ESP32-S3 target
```
cp sdkconfig.defaults.esp32s3.debug sdkconfig.defaults
idf.py set-target esp32s3
idf.py menuconfig
```
### Set ESP32-S2 Mini target
```
cp sdkconfig.defaults.esp32s2.debug sdkconfig.defaults
idf.py set-target esp32s2
idf.py menuconfig
```

# Quick Reference

## Clone repo
  `git clone --recursive git@github.com:Strefa35/esp-platform.git esp/esp-platform`

## ESP-IDF Extra Components
  `git clone --recursive git@github.com:espressif/idf-extra-components.git esp/idf-extra-components`

## ESP drivers
  `git clone git@github.com:Strefa35/esp-drivers.git esp/esp-drivers`

## Configure ESP environment

### Install ESP-IDF
- To build repo you need to install [Espressif IoT Development Framework](https://github.com/espressif/esp-idf)
- More information you can find here: [Installation Step by Step](hhttps://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/linux-macos-setup.html#installation-step-by-step)

- From esp-idf folder do below command: `. ./export.sh` to export 

For detailed build and compilation instructions, see [docs/build.md](docs/build.md).

For **runtime heap snapshot logging** (optional `mem_check` module, menuconfig, and serial log format), see [docs/memory.md](docs/memory.md). To turn a capture into Markdown tables of approximate per-module deltas, see [docs/parse_mem_log.md](docs/parse_mem_log.md).

### Configuration esp-platform
- `idf.py menuconfig`

### Build esp-platform
- `idf.py build`

### Run esp-platform
- Check which USB port you connected the ESP to:
  - `ls -la /dev/tty*`

- Flash the image and display serial output:
  - `idf.py -p /dev/ttyUSB0 flash monitor` - for USB A
  - `idf.py -p /dev/ttyACM0 flash monitor` - for USB C

# ESP Repo

## List of modules
The solution consists of several modules:
- mgr_ctrl
- eth_ctrl
- mqtt_ctrl
- relay_ctrl
- sensor_ctrl
- sys_ctrl

The main module is the management module (**mgr_ctrl**) which allows for managing modules and mediates data transmission between modules.

# Communication

The MQTT protocol is used for bidirectional communication between the Controller and the outside world. This allows you to send information from the controller through the **MQTT Broker** to any other module that has also connected to the broker, as well as send queries from an external module to the controller. Data is sent in **JSON format**.

For detailed MQTT protocol documentation including all modules and message formats, see [docs/mqtt.md](docs/mqtt.md).