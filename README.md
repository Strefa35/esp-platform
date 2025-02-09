# Electric Water Heater Controller

The idea of ​​the project was created to control the switching on of an electric water heater based on the intensity of sunlight. This was to enable the use of the surplus energy produced by the solar panels to heat utility water in a single-family house. 

In order to fully automate this process, it was necessary to build a controller that would turn the power supply of the electric water heater on or off when the solar panels produce the most energy. Its excess is discharged into the power company's network.

## Assumptions
- simple and cheap construction
- easy configuration, also remote
- easy expansion
- possibility of connecting various sensors (temperature, humidity, light intensity, etc.)

# Quick Reference

## Clone repo
  `git clone --recursive git@github.com:Strefa35/esp-heater-controller.git`

## Build

### Install ESP-IDF
- To build repo you need to install [Espressif IoT Development Framework](https://github.com/espressif/esp-idf)
- More information you can find here: [Installation Step by Step](hhttps://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/linux-macos-setup.html#installation-step-by-step)

- From esp-idf folder do below command: `. ./export.sh` to export 

### Build Electric Water Heater Controller
- `idf.py build`

