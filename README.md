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
- [ESP32-EVB](https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)
- [ESP32-S3-ETH](https://www.waveshare.com/wiki/ESP32-S3-ETH)

The above boards have their configurations available in sdkconfig.defaults files:
- sdkconfig.defaults.esp32.debug
- sdkconfig.defaults.esp32s3.debug

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
### Build selected target
`idf.py menuconfig`

## List of modules
The solution consists of several modules:
- mgr_ctrl
- eth_ctrl
- mqtt_ctrl
- relay_ctrl
- sensor_ctrl

The main module is the management module (**mgr_ctrl**) which allows for managing modules and mediates data transmission between modules.

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

### Configuration esp-platform
- `idf.py menuconfig`

### Build esp-platform
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

## REGISTER/ESP
Information about connected devices

After starting, the controller sends information about its configuration.
Then the controller subscribes to the topic list for each resource.

#### Request
- Topic: `REGISTER/ESP`
- Operation: `get`
- Get current controller configuration:
  ```
  {
    "operation": "get"
  }
  ```
#### Response
- Topic: `REGISTER/ESP`, 
- Operation: `response`
- Response after get:
  ```
  {
    "operation": "response",
    "uid": "ESP/12AB34",
    "mac": "12:34:56:78:90:AB",
    "ip": "10.0.0.20",
    "list": ["eth", "mqtt"]
  }
  ```

#### Event
- Topic: `REGISTER/ESP`
- Operation: `event`
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

## MQTT

#### Request
- Topic: `ESP/12AB34/req/mqtt`, 
- Operation: `set/get`
- Set broker ip and port:
  ```
  {
    "operation": "set",
    "broker": { 
      "address": {
        "uri": "mqtt-broker-uri",
        "port": value
      }
    }
  }
  ```
- Get current state for mqtt:
  ```
  {
    "operation": "get",
  }
  ```
#### Response
- Topic: `ESP/12AB34/res/mqtt`, 
- Operation: `response`
- Response after get:
  ```
  {
    "operation": "response",
    "broker": { 
      "address": {
        "uri": "mqtt-broker-uri",
        "port": value
      }
    }
  }
  ```

## RELAY

#### Request
- Topic: `ESP/12AB34/req/relay`, 
- Operation: `set/get`
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
#### Response
- Topic: `ESP/12AB34/res/relay`, 
- Operation: `response`
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
#### Event
- Topic: `ESP/12AB34/event/relay`, 
- Operation: `event`
- Event for specific relays:
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

## Sensor

#### Request
- Topic: `ESP/12AB34/req/sensor`, 
- Operation: `set/get`
- Set:
  ```
  {
    "operation": "set",
    "sensor": "name-of-sensor",
    "data": []
  }
  ```
  ```
  "data": [
    {
      "type": "threshold",
      "threshold": value
    },
    {
      "type": "lux",
      "lux": value
    },
    {
      "type": "info",
      "info": {}
    },
  ]
  ```
- Get list of parameters for specific sensor:
  ```
  {
    "operation": "get",
    "sensor": "name-of-sensor",
    "data": ["info", "threshold", "lux", ...]
  }
  ```
#### Response
- Topic: `ESP/12AB34/res/sensor`, 
- Operation: `response`
- Sensor: `name-of-sensor`
- Response after get:
  ```
  {
    "operation": "response",
    "sensor": "name-of-sensor",
    "data": []
  }
  ```
  ```
  "data": [
    {
      "type": "threshold",
      "threshold": value
    },
    {
      "type": "lux",
      "lux": value
    },
    {
      "type": "info",
      "info": {}
    },
  ]
  ```
#### Event
- Topic: `ESP/12AB34/event/sensor`, 
- Operation: `event`
- Sensor: `name-of-sensor`
- Event for specific sensor:
  ```
  {
    "operation": "event",
    "sensor": "name-of-sensor",
    "data": []
  }
  ```
  ```
  "data": [
    {
      "type": "threshold",
      "threshold": value
    },
    {
      "type": "lux",
      "lux": value
    },
    {
      "type": "info",
      "info": {}
    },
  ]
  ```
