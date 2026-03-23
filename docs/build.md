# Prepare platform depend on MCU (target)
To show list of supported targets:
```idf.py --list-targets```

## ESP32-EVT
  - ```idf.py set-target esp32```
## ESP32-S2
###
  - [Establish Serial Connection with ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s2/get-started/establish-serial-connection.html?highlight=some%20serial%20port%20wiring%20configurations)
  - [Build the Project (Demo)](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s2/get-started/start-project.html)

  - ```idf.py set-target esp32s2```

# Build platform
- ```idf.py menuconfig```
- ```idf.py build```

# Flash & run
## ESP32-EVT
- ```idf.py -p /dev/ttyUSB0 flash monitor```

## ESP32-S2
- ```idf.py -p /dev/ttyACM0 flash monitor```
- https://docs.espressif.com/projects/esp-idf/en/v4.3-beta2/esp32s2/get-started/index.html#get-started-connect

# Generate a sdkconfig.defaults
- ```idf.py save-defconfig```
