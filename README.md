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

The solution consists of several modules:
- mgr_ctrl
- eth_ctrl
- mqtt_ctrl
- relay_ctrl
- sensor_ctrl

The main module is the management module (mgr_ctrl) which allows for managing modules and mediates data transmission between modules.

# Quick Reference

## Clone repo
  `git clone --recursive git@github.com:Strefa35/esp-platform.git esp/esp-platform`

## ESP-IDF Extra Components
  `git clone --recursive git@github.com:espressif/idf-extra-components.git esp/idf-extra-components`

## ESP drivers
  `git clone git@github.com:Strefa35/esp-drivers.git esp/esp-drivers`

## Build

### Install ESP-IDF
- To build repo you need to install [Espressif IoT Development Framework](https://github.com/espressif/esp-idf)
- More information you can find here: [Installation Step by Step](hhttps://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/linux-macos-setup.html#installation-step-by-step)

- From esp-idf folder do below command: `. ./export.sh` to export 

### Build Electric Water Heater Controller
- `idf.py build`

# Communication

The MQTT protocol is used for bidirectional communication between the Controller and the outside world. This allows you to send information from the controller through the **MQTT Broker** to any other module that has also connected to the broker, as well as send queries from an external module to the controller. Data is sent in **JSON format**.

## List of topic/key

The **MQTT** protocol consists of two parts: 
- **topic**
- **key**

More information [here](https://mqtt.org/).

Topic is used to inform which resource is the sender or receiver of the information. The key field is used to pass this information and the **JSON format** is used. The most important field is **operation** which specifies the type of operation related to the data that occurs there.

We distinguish the following types of operations:
- event - information about new data
- set - set request
- get - get request
- response - set/get response

### REGISTER/ESP

After starting, the controller sends information about its configuration.
Then the controller subscribes to the topic list for each resource resource.

Topic: `REGISTER/ESP`, Operation: `get/event`

- Notification about controller configuration and list of available modules:
  ```
  {
    "operation": "event",
    "uid": "ESP/12AB34",
    "mac": "12:34:56:78:90:AB",
    "ip": "10.0.0.20",
    "list": ["eth", "mqtt"]
  }
  ```

- Get current controller configuration:
  ```
  {
    "operation": "get"
  }
  ```

### ESP/12AB34/relay

Topic: `ESP/12AB34/relay`, Operation: `set/get/event/response`

- Set state for specific relay number:
  ```
  {
    "operation": "set",
    "relays": [
      { 
        "number": 0 or 1,
        "state": "on/off"
      },
    ]
  }
  ```
- Get current state for specific relay number:
  ```
  {
    "operation": "get",
  }
  ```
- Response after get:
  ```
  {
    "operation": "response",
    "relays": [
      { 
        "number": 0 or 1,
        "state": "on/off"
      },
    ]
  }
  ```
- Event notification for specific relays:
  ```
  {
    "operation": "event",
    "relays": [
      { 
        "number": 0 or 1,
        "state": "on/off"
      },
    ]
  }
  ```

### ESP/12AB34/sensor

Topic: `ESP/12AB34/sensor`, Operation: `set/get/event/response`

- Set state for specific relay number:
  ```
  {
    "operation": "set",
    "sensor": "name-of-sensor",
    "threshold": {
      "min": value,
      "max": value
    }
  }
  ```
- Get list of parameters for specific sensor:
  ```
  {
    "operation": "get",
    "sensor": "name-of-sensor",
    "list": ["info", threshold", "lux", ...]
  }
  ```
- Response after get:
  ```
  {
    "operation": "response",
    "sensor": "name-of-sensor",
    "info": {
      
    },
    "threshold": {
      "min": value,
      "max": value
    },
    "lux": value,
  }
  ```
- Event notification for specific sensor:
  ```
  {
    "operation": "event",
    "sensor": "name-of-sensor",
    "info": {
      
    },
    "threshold": {
      "min": value,
      "max": value
    },
    "lux": value,
  }
  ```
